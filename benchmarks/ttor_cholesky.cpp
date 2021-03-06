#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <fstream>
#include <array>
#include <random>
#include <mutex>
#include <iostream>
#include <map>
#include <tuple>
#include <mpi.h>

#include "tasktorrent.hpp"
#include "bench_common.h"

using namespace std;
using namespace Eigen;
using namespace ttor;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

int VERB = 0;
bool LOG = false;
int n_threads_ = 4;
int n_ = 128;
int N_ = 8;
int p_ = 1;
int q_ = 1;

void cholesky(int n_threads, int n, int N, int p, int q)
{
    // MPI info
    const int rank = comm_rank();
    const int n_ranks = comm_size();
    if(VERB) printf("[%d] Hello from %s\n", comm_rank(), processor_name().c_str());

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
        return static_cast<double>(N*N - ((i*(i-1))/2 + j));
    };

    // Block the matrix for every node
    // Store it in a map
    // Later on we can probably optimize this, to avoid storing the whole thing
    // Note: concurrent access to the map should _NOT_ modify it, otherwise we need a lock
    map<int2, MatrixXd> Mat;
    for(int i = 0; i < N; i++) {
        for(int j = 0; j <= i; j++) {
            if(block2rank({i,j}) == rank) {
                Mat[{i,j}] = A.block(i * n, j * n, n, n);
            } else {
                Mat[{i,j}] = MatrixXd::Zero(n, n);
            }
        }
    }

    // Factorize
    {
        // Initialize the communicator structure
        Communicator comm(MPI_COMM_WORLD, VERB);

        // Threadpool
        Threadpool tp(n_threads, &comm, VERB, "[" + to_string(rank) + "]_");
        Taskflow<int> potf_tf(&tp, VERB);
        Taskflow<int2> trsm_tf(&tp, VERB);
        Taskflow<int3> gemm_tf(&tp, VERB);

        // Log
        DepsLogger dlog(1000000);
        Logger log(1000000);
        if(LOG) {
            tp.set_logger(&log);
            comm.set_logger(&log);
        }
        
        // Active messages
        // Sends a pivot and trigger multiple trsms in one columns
        auto am_trsm = comm.make_active_msg( 
            [&](view<double> &Lkk, int& j, view<int>& is) {
                Mat.at({j,j}) = Map<MatrixXd>(Lkk.data(), n, n);
                for(auto& i: is) {
                    trsm_tf.fulfill_promise({i,j});
                }
            });

        // Sends a panel bloc and trigger multiple gemms
        auto am_gemm = comm.make_large_active_msg(
            [&](int&, int& j, view<int2>& ijs) {
                for(auto& ij: ijs) {
                    int gi = ij[0];
                    int gj = ij[1];
                    int gk = j;
                    gemm_tf.fulfill_promise({gi,gj,gk});
                }
            },
            [&](int& i, int& j, view<int2>&) {
                return Mat.at({i,j}).data();
            },
            [&](int&, int&, view<int2>&) {
                return;
            });


        // potf 
        potf_tf.set_mapping([&](int j) {
                return (j % n_threads);
            })
            .set_indegree([](int) {
                return 1;
            })
            .set_task([&](int j) {
                LLT<Ref<MatrixXd>> llt(Mat.at({j,j}));
                assert(llt.info() == Eigen::Success);
            })
            .set_fulfill([&](int j) {
                // Dependencies
                map<int,vector<int>> to_fulfill; // rank -> trsm[i,j] on that rank at (i,j)
                for(int i = j+1; i<N; i++) {
                    int r = block2rank({i,j});
                    if(to_fulfill.count(r) == 0) {
                        to_fulfill[r] = {i};
                    } else {
                        to_fulfill[r].push_back(i);
                    }
                }
                // Send data & trigger tasks
                for(auto& p: to_fulfill) {
                    int r = p.first;
                    // Task is local. Just fulfill.
                    if(r == rank) {
                        for(auto& i: p.second) {
                            trsm_tf.fulfill_promise({i,j});
                        }
                    // Task is remote. Send data and fulfill.
                    } else {
                        auto Ljjv = view<double>(Mat.at({j,j}).data(), n*n);
                        auto isv = view<int>(p.second.data(), p.second.size());
                        am_trsm->send(r, Ljjv, j, isv);
                    }
                }
            })
            .set_name([](int j) {
                return "potf_" + to_string(j);
            })
            .set_priority([&](int j) {
                return block2prio({j,j});
            });

        // trsm
        trsm_tf.set_mapping([&](int2 ij) {
                return ((ij[0] + ij[1] * N) % n_threads);
            })
            .set_indegree([](int2 ij) {
                return (ij[1] == 0 ? 0 : 1) + 1;
            })
            .set_task([&](int2 ij) {
                int i = ij[0];
                int j = ij[1];
                auto L = Mat.at({j,j}).triangularView<Lower>().transpose();
                Mat.at({i,j}) = L.solve<OnTheRight>(Mat.at({i,j}));
            })
            .set_fulfill([&](int2 ij) {
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
                            gemm_tf.fulfill_promise({gi,gj,gk});
                        }
                    // Task is remote. Send data and fulfill.
                    } else {
                        auto Lijv = view<double>(Mat.at({i,j}).data(), n*n);
                        auto ijsv = view<int2>(p.second.data(), p.second.size());
                        am_gemm->send_large(r, Lijv, i, j, ijsv);
                    }
                } 
            })
            .set_name([](int2 ij) {
                return "trsm_" + to_string(ij[0]) + "_" + to_string(ij[1]);
            })
            .set_priority([&](int2 ij) {
                return block2prio(ij);
            });

        // gemm
        gemm_tf.set_mapping([&](int3 ijk) {
                return ((ijk[0] + ijk[1] * N + ijk[2] * N * N) % n_threads);
            })
            .set_indegree([](int3 ijk) {
                int i = ijk[0];
                int j = ijk[1];
                int k = ijk[2];
                return (k == 0 ? 0 : 1) + (i == j ? 1 : 2);
            })
            .set_task([&](int3 ijk) {
                int i = ijk[0];
                int j = ijk[1];
                int k = ijk[2];
                Mat.at({i,j}).noalias() -= Mat.at({i,k}) * Mat.at({j,k}).transpose();
                assert(k < N - 1);
            })
            .set_fulfill([&](int3 ijk) {
                int i = ijk[0];
                int j = ijk[1];
                int k = ijk[2];
                if (k + 1 == i && k + 1 == j)
                {
                    potf_tf.fulfill_promise(k+1); // same node, no comms
                }
                else if (k + 1 == j)
                {
                    trsm_tf.fulfill_promise({i, k + 1}); // same node, no comms
                }
                else
                {
                    gemm_tf.fulfill_promise({i, j, k + 1}); // same node, no comms
                }
            })
            .set_name([](int3 ijk) {
                return "gemm_" + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
            })
            .set_priority([&](int3 ijk) {
                return block2prio({ijk[0], ijk[1]});
            });

            if(rank == 0 && VERB) printf("Starting Cholesky\n");
            MPI_Barrier(MPI_COMM_WORLD);
            double t0 = getWallTime();
            if (rank == 0){
                potf_tf.fulfill_promise(0);
            }
            tp.join();
            MPI_Barrier(MPI_COMM_WORLD);
            double t1 = getWallTime();
            if(rank == 0)
            {
              printf("ttor_cholesky nranks: %d [%dx%d] nthreads: %d N: %d n: %d time: %.2lf\n",
                     n_ranks, p, q, n_threads, N, n, t1-t0);
            }

            if(LOG) {
                std::ofstream logfile;
                string filename = "cholesky_"+ to_string(n_ranks)+".log."+to_string(rank);
                logfile.open(filename);
                logfile << log;
                logfile.close();
            }
    }

#ifndef NDEBUG
    // Gather everything on rank 0 and test for accuracy
    {
        Communicator comm(MPI_COMM_WORLD, VERB);
        Threadpool tp(n_threads, &comm, VERB);
        Taskflow<int2> gather_tf(&tp, VERB);
        auto am_gather = comm.make_active_msg(
        [&](view<double> &Lij, int& i, int& j) {
        	A.block(i * n, j * n, n, n) = Map<MatrixXd>(Lij.data(), n, n);
        });
        // potf 
        gather_tf.set_mapping([&](int2 ij) {
                return ( (ij[0] + ij[1]) % n_threads );
            })
            .set_indegree([](int2) {
                return 1;
            })
            .set_task([&](int2 ij) {
                int i = ij[0];
                int j = ij[1];
                if(rank != 0) {
                    auto Lijv = view<double>(Mat.at({i,j}).data(), n*n);
                    am_gather->send(0, Lijv, i, j);
                } else {
                    A.block(i * n, j * n, n, n) = Mat.at({i,j});
                }
            })
            .set_name([](int2 ij) {
                return "gather_" + to_string(ij[0]) + "_" + to_string(ij[1]);
            });

        for(int i = 0; i < N; i++) {
            for(int j = 0; j <= i; j++) {
                if(block2rank({i,j}) == rank) {
                    gather_tf.fulfill_promise({i,j});
                }
            }
        }
        tp.join();
        MPI_Barrier(MPI_COMM_WORLD);

        if(rank == 0) {
            // Test 1         
            {
                auto L = A.triangularView<Lower>();
                VectorXd x = VectorXd::Random(n * N);
                VectorXd b = Aref*x;
                VectorXd bref = b;
                L.solveInPlace(b);
                L.transpose().solveInPlace(b);
                double error = (b - x).norm() / x.norm();
                if (error >= 1e-10) {
                  cout << "Error solve: " << error << endl;
                  exit(EXIT_FAILURE);
                }
            }
            // Test 2
            {
                // MatrixXd L = A.triangularView<Lower>();
                // MatrixXd A = L * L.transpose();
                // double error = (A - Aref).norm() / Aref.norm();
                // cout << "Error LLT : " << error << endl;
                // EXPECT_LE(error, 1e-8);
            }
        }
    }
#endif
}

int main(int argc, char **argv)
{
    int req = MPI_THREAD_FUNNELED;
    int prov = -1;

    MPI_Init_thread(NULL, NULL, req, &prov);

    assert(prov == req);

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

    MPI_Finalize();

    return 0;
}
