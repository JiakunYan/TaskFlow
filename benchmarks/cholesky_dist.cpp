#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <fstream>
#include <array>
#include <random>
#include <mutex>
#include <iostream>
#include <map>
#include <tuple>

#include "tf.hpp"
#include "bench_common.h"
using namespace std;
using namespace Eigen;
using namespace tf;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

int VERB = 0;
int n_threads_ = 4;
int n_ = 128;
int N_ = 8;
int p_ = 1;
int q_ = 1;

void cholesky(int n_threads, int n, int N, int p, int q)
{
  tf::Context context(n_threads);
  tf::Communicator comm;

  // MPI info
  const int rank = comm.rank_me();
  const int n_ranks = comm.rank_n();

  if (p * q != n_ranks) {
    for (p = (int)sqrt((double)n_ranks); p > 0 && n_ranks % p != 0; --p) continue;
    q = n_ranks / p;
  }
  assert(p * q == n_ranks);
  assert(p >= 1);
  assert(q >= 1);

  // Form the matrix : let every node have a copy of A for now
  auto gen = [&](int i, int j) {
    if(i == j) {
      return static_cast<double>(N*n+2);
    } else {
      int k = (i+j)%3;
      if(k == 0) {
        return 0.0;
      } else if (k == 1) {
        return 0.5;
      } else {
        return 1.0;
      }
    }
  };
  MatrixXd A = MatrixXd::NullaryExpr(N * n, N * n, gen);
  MatrixXd Aref = A;
  A = A.triangularView<Lower>();

  // Mapper
  auto block2rank = [&](int2 ij){
    int i = ij[0];
    int j = ij[1];
    int ii = i % p;
    int jj = j % q;
    int r = ii + jj * p;
    assert(r <= n_ranks);
    return r;
  };

  // Gives the priority
  auto block2prio = [&](int2 ij) {
    int i = ij[0]+1;
    int j = ij[1]+1;
    assert(i >= j);

    double current_prio = static_cast<double>(N*N - ((i*(i-1))/2 + j));

    int priority = 0;
    if (current_prio < 0.1 * N * N) {
      priority = 0;
    } else if (current_prio < 0.3 * N * N) {
      priority = 1;
    } else if (current_prio < 0.5 * N * N) {
      priority = 2;
    } else {
      priority = 3;
    }

    return priority;
  };

  // Block the matrix for every node
  // Store it in a map
  // Later on we can probably optimize this, to avoid storing the whole thing
  // Note: concurrent access to the map should _NOT_ modify it, otherwise we need a lock
  map<int2, MatrixXd> Mat;
  for(int i = 0; i < N; i++) {
    for (int j = 0; j <= i; j++) {
      if (block2rank({i, j}) == rank) {
        Mat[{i, j}] = A.block(i * n, j * n, n, n);
      } else {
        Mat[{i, j}] = MatrixXd::Zero(n, n);
      }
    }
  }

  // Factorize
  {
    // Threadpool
    TaskClass<int> potf_tf;
    TaskClass<int2> trsm_tf;
    TaskClass<int3> gemm_tf;

    // Active messages
    // Sends a pivot and trigger multiple trsms in one columns
    std::function<void(view<double> &, int&, view<int>&)> fn_trsm =
        [&](view<double> &Lkk, int& j, view<int>& is) {
          Mat.at({j,j}) = Map<MatrixXd>(Lkk.data(), n, n);
          assert(Mat.at({j,j}).data() != Lkk.data());
          for(auto& i: is) {
            context.signal(trsm_tf, {i,j});
          }
        };
    auto am_trsm = comm.makeActiveMsg(fn_trsm);

    // Sends a panel bloc and trigger multiple gemms
    std::function<void(view<double>&, int&, int&, view<int2>&)> fn_gemm =
        [&](view<double> Lijv, int& i, int& j, view<int2>& ijs) {
          Mat.at({i,j}) = Map<MatrixXd>(Lijv.data(), n, n);
          assert(Mat.at({i,j}).data() != Lijv.data());
          for(auto& ij: ijs) {
            int gi = ij[0];
            int gj = ij[1];
            int gk = j;
            context.signal(gemm_tf, {gi,gj,gk});
          }
        };
    auto am_gemm = comm.makeActiveMsg(fn_gemm);

    std::function<void()> fn_term =
        [&]() {
          context.signalTerm();
        };
    auto am_term = comm.makeActiveMsg(fn_term);


    // potf
    potf_tf
        .setAffinity([&](int j) {
          return (j % n_threads);
        })
        .setInDep([](int) {
          return 1;
        })
        .setTask([&](int j) {
          LLT<Ref<MatrixXd>> llt(Mat.at({j,j}));
          assert(llt.info() == Eigen::Success);
        })
        .setOutDep([&](int j) {
          // Dependencies
          if (j == N - 1) {
            // done
            context.signalTerm();
            for (int i = 0; i < comm.rank_n(); ++i) {
              if (i != comm.rank_me())
                am_term.send(i);
            }
          } else {
            // signal more tasks
            map<int, vector<int>>
                to_fulfill; // rank -> trsm[i,j] on that rank at (i,j)
            for (int i = j + 1; i < N; i++) {
              int r = block2rank({i, j});
              if (to_fulfill.count(r) == 0) {
                to_fulfill[r] = {i};
              } else {
                to_fulfill[r].push_back(i);
              }
            }
            // Send data & trigger tasks
            for (auto &p : to_fulfill) {
              int r = p.first;
              // Task is local. Just fulfill.
              if (r == rank) {
                for (auto &i : p.second) {
                  context.signal(trsm_tf, {i, j});
                }
                // Task is remote. Send data and fulfill.
              } else {
                auto Ljjv = view<double>(Mat.at({j, j}).data(), n * n);
                auto isv = view<int>(p.second.data(), p.second.size());
                am_trsm.send(r, Ljjv, j, isv);
              }
            }
          }
        })
        .setName([](int j) {
          return string("POTF at ") + to_string(j);
        })
        .setPriority([&](int j) {
          return block2prio({j, j});
        });

    // trsm
    trsm_tf
        .setAffinity([&](int2 ij) {
          return ((ij[0] + ij[1] * N) % n_threads);
        })
        .setInDep([](int2 ij) {
          return (ij[1] == 0 ? 0 : 1) + 1;
        })
        .setTask([&](int2 ij) {
          int i = ij[0];
          int j = ij[1];
          auto L = Mat.at({j,j}).triangularView<Lower>().transpose();
          Mat.at({i,j}) = L.solve<OnTheRight>(Mat.at({i,j}));
        })
        .setOutDep([&](int2 ij) {
          int i = ij[0];
          int j = ij[1];
          // Dependencies
          map<int,vector<int2>> to_fulfill; // rank -> gemm[i,j,k] on that rank at (i,j)
          for (int k = j+1; k<=i; k++) // on the right, row i, cols k=j+1, ..., i
          {
            int r = block2rank({i,k});
            if(to_fulfill.count(r) == 0) {
              to_fulfill[r] = { {i,k} };
            } else {
              to_fulfill[r].push_back({i,k});
            }
          }
          for (int k = i+1; k<N; k++) // below, col i, row k=i+1, ..., N
          {
            int r = block2rank({k,i});
            if(to_fulfill.count(r) == 0) {
              to_fulfill[r] = { {k,i} };
            } else {
              to_fulfill[r].push_back({k,i});
            }
          }
          // Send data & trigger tasks
          for(auto& p: to_fulfill) {
            int r = p.first;
            // Task is local. Just fulfill.
            if(r == rank) {
              for(auto& ij: p.second) {
                int gi = ij[0];
                int gj = ij[1];
                int gk = j;
                context.signal(gemm_tf, {gi,gj,gk});
              }
              // Task is remote. Send data and fulfill.
            } else {
              auto Lijv = view<double>(Mat.at({i,j}).data(), n*n);
              auto ijsv = view<int2>(p.second.data(), p.second.size());
              am_gemm.send(r, Lijv, i, j, ijsv);
            }
          }
        })
        .setName([](int2 ij) {
          return string("TRSM at ") + to_string(ij[0]) + "_" + to_string(ij[1]);
        })
        .setPriority([&](int2 ij) {
          return block2prio(ij);
        });

    // gemm
    gemm_tf.setAffinity([&](int3 ijk) {
          return ((ijk[0] + ijk[1] * N + ijk[2] * N * N) % n_threads);
        })
        .setInDep([](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];
          return (k == 0 ? 0 : 1) + (i == j ? 1 : 2);
        })
        .setTask([&](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];
          Mat.at({i,j}).noalias() -= Mat.at({i,k}) * Mat.at({j,k}).transpose();
          assert(k < N - 1);
        })
        .setOutDep([&](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];
          if (k + 1 == i && k + 1 == j)
          {
            context.signal(potf_tf, i);
          }
          else if (k + 1 == j)
          {
            context.signal(trsm_tf, {i, j});
          }
          else
          {
            context.signal(gemm_tf, {i, j, k+1});
          }
        })
        .setName([](int3 ijk) {
          return string("GEMM at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
        })
        .setPriority([&](int3 ijk) {
          return block2prio({ijk[0], ijk[1]});
        });

    comm.barrier();
    double t0 = getWallTime();
    context.start(1);
    if (rank == 0){
      context.signal(potf_tf, 0);
    }
    while (!context.tryJoin()) {
      comm.progress();
    }
    comm.drain();
    comm.barrier();
    double t1 = getWallTime();

    if(rank == 0) {
      printf("cholesky_dist nranks: %d [%dx%d] nthreads: %d N: %d n: %d time: %.2lf\n",
             n_ranks, p, q, n_threads, N, n, t1-t0);
    }
  }

#ifndef NDEBUG
  // Gather everything on rank 0 and test for accuracy
  {
    int recv_num = 0;
    TaskClass<int2> gather_tf;
    std::function<void(view<double> &, int&, int&)> fn_gather =
        [&](view<double> &Lij, int& i, int& j) {
          A.block(i * n, j * n, n, n) = Map<MatrixXd>(Lij.data(), n, n);
          ++recv_num;
        };
    auto am_gather = comm.makeActiveMsg(fn_gather);
    // gather
    gather_tf
        .setInDep([](int2) {
          return 1;
        })
        .setTask([&](int2 ij) {
          int i = ij[0];
          int j = ij[1];
          if(rank != 0) {
            auto Lijv = view<double>(Mat.at({i,j}).data(), n*n);
            am_gather.send(0, Lijv, i, j);
          } else {
            A.block(i * n, j * n, n, n) = Mat.at({i,j});
          }
        })
        .setName([](int2 ij) {
          return string("GATHER at ") + to_string(ij[0]) + "_" + to_string(ij[1]);
        });

    int to_recv = 0;
    for(int i = 0; i < N; i++) {
      for(int j = 0; j <= i; j++) {
        if(block2rank({i,j}) == rank) {
          context.signal(gather_tf, {i,j});
        } else {
          ++to_recv;
        }
      }
    }
    context.start();
    while (!context.tryJoin()) {
      comm.progress();
    }
    while (rank == 0 && recv_num < to_recv) comm.progress();
    comm.drain();
    comm.barrier();

    if(rank == 0) {
      // Test 1
      auto L = A.triangularView<Lower>();
      VectorXd x = VectorXd::Random(n * N);
      VectorXd b = Aref*x;
      L.solveInPlace(b);
      L.transpose().solveInPlace(b);
      double error = (b - x).norm() / x.norm();
      if (error >= 1e-10) {
        cout << "Error solve: " << error << endl;
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}


int main(int argc, char **argv)
{
  if (argc >= 2)
  {
    n_threads_ = atoi(argv[1]);
  }

  if (argc >= 3)
  {
    n_ = atoi(argv[2]);
  }

  if (argc >= 4)
  {
    N_ = atoi(argv[3]);
  }

  if (argc >= 5)
  {
    p_ = atoi(argv[4]);
  }

  if (argc >= 6)
  {
    q_ = atoi(argv[5]);
  }

  if (argc >= 7)
  {
    VERB = atoi(argv[6]);
  }

  int n_threads = n_threads_;
  int n = n_;
  int N = N_;
  int p = p_;
  int q = q_;

  cholesky(n_threads, n, N, p, q);

  return 0;
}
