#include <iostream>
#include <array>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include <map>
#include <tuple>
#include <fstream>
#include <array>
#include <random>
#include <mutex>
#include "tf.hpp"
#include "bench_common.h"

using namespace std;
using namespace tf;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

#define DTYPE double
#define WEIGHT_1D(jj) weight_1D[jj+R]
#define IN_2D(i,j,k)  A[k][j*M+i]
#define OUT_2D(i,j,k) A[k][j*N+i]

// Global Constant
int VERB = 0;
int n_threads_ = 4;
int M_ = 100;
int N_ = 100;
int mb_ = 10; // width of block
int nb_ = 10; // height of block
int p_ = 1;
int q_ = 1;


void stencil_1D(int n_threads, int M, int N, int mb, int nb, int p, int q){
  int lm = M/mb;
  int ln = N/nb;
  int block_num = lm * ln;
  int iter = 10;
  int R = 1;

  /* intialize weight_1D */
  DTYPE *weight_1D = (DTYPE *)malloc(sizeof(DTYPE) * (2*R+1));
  for(int jj = 1; jj <= R; jj++) {
    WEIGHT_1D(jj) = (DTYPE)(1.0/(2.0*jj*R));
    WEIGHT_1D(-jj) = -(DTYPE)(1.0/(2.0*jj*R));
  }
  WEIGHT_1D(0) = (DTYPE)1.0;

  // Create an matrix A
  // DTYPE *A = (DTYPE *)malloc(sizeof(DTYPE) * (mb*nb));
  // DTYPE *out_A = (DTYPE *)malloc(sizeof(DTYPE) * (mb*nb));
  DTYPE **A;
  A = (DTYPE **)malloc(iter * sizeof(DTYPE *));
  for(int i = 0; i < iter; i++) {
    A[i] = (DTYPE *)malloc((M*N) * sizeof(DTYPE));
  }

  for(int k = 0; k < iter; k++)
    for(int j = R; j < N - R; j++)
        for(int i = 0; i < M; i++)
            A[k][j*M+i] = (DTYPE)1.0 * i + (DTYPE)1.0 * j;

  for(int k = 0; k < iter; k++)
    for(int j = 0; j < R; j++)
      for(int i = 0; i < M; i++)
          A[k][j*M+i] = (DTYPE)0.0;

  for(int k = 0; k < iter; k++)
    for(int j = N - R; j < N; j++)
        for(int i = 0; i < M; i++)
            A[k][j*M+i] = (DTYPE)0.0;




  tf::Context context(n_threads);
  tf::Communicator comm;

  // MPI info
  const int rank = comm.rank_me();
  const int n_ranks = comm.rank_n();
  printf("%d and %d\n", rank, n_ranks);

  if (p * q != n_ranks) {
    for (p = (int)sqrt((double)n_ranks); p > 0 && n_ranks % p != 0; --p) continue;
    q = n_ranks / p;
  }
  assert(p * q == n_ranks);
  assert(p >= 1);
  assert(q >= 1);

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

  // Factorize
  {
    // define tasks
    tf::TaskClass<int3> stencil;

    // Sends a panel bloc and trigger multiple gemms
    std::function<void(view<DTYPE>&, int&, int&, int&, view<int2>&)> fn_stencil =
        [&](view<DTYPE> Lijv, int& i, int& j, int& k, view<int2>& ijs) {
          int thread_i_start = i * mb;
          int thread_i_end = (i+1) * mb;
          int thread_j_start = j * nb;
          int thread_j_end = (j+1) * nb;

          int jj = 0;
          int ii = 0;
          for(int thread_j = thread_j_start; thread_j < thread_j_end; thread_j++) {
            for(int thread_i = thread_i_start; thread_i < thread_i_end; thread_i++) {
              IN_2D(thread_i, thread_j, k) = Lijv.data()[jj*mb+ii];
              ii++;
            }
            jj++;
          }

          for(auto& ij: ijs) {
            int gi = ij[0];
            int gj = ij[1];
            int gk = k+1;
            context.signal(stencil, {gi,gj,gk});
          }
        };
    auto am_stencil = comm.makeActiveMsg(fn_stencil);

    std::function<void()> fn_term =
        [&]() {
          context.signalTerm();
        };
    auto am_term = comm.makeActiveMsg(fn_term);

    stencil.setAffinity([&](int3 ijk) {
          return ((ijk[0] + ijk[1] * M) % n_threads);
        })
        .setTask([&](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];

          int thread_i_start = i * mb;
          int thread_i_end = (i+1) * mb;
          int thread_j_start = j * nb;
          int thread_j_end = (j+1) * nb;;

          printf("%d %d %d: %d and %d\n", ijk[0], ijk[1], ijk[2], thread_i_start, thread_j_start);
          printf("%d %d %d: %d and %d\n", ijk[0], ijk[1], ijk[2], thread_i_end, thread_j_end);

          for(int thread_j = thread_j_start; thread_j < thread_j_end; thread_j++) {
            for(int thread_i = thread_i_start; thread_i < thread_i_end; thread_i++) {
              //printf("%d and %d\n", thread_j, thread_i); 
              if(thread_j >= R && thread_j < N - R) {
                OUT_2D(thread_i, thread_j, k) = WEIGHT_1D(-R) * IN_2D(thread_i, thread_j-R, k-1);
                for (int jj = -R+1; jj <= R; jj++) {
                    OUT_2D(thread_i, thread_j, k) += WEIGHT_1D(jj) * IN_2D(thread_i, thread_j+jj, k-1);
                }
              }
            }
          }

        })
        .setInDep([&](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];

          if(k == 1) {
            return 1;
          }

          if( (j == 0 && i == 0) || (j == 0 && i == lm-1) || (j == ln-1 && i == 0) || (j == ln-1 && i == lm-1) ) {
            return 3;
          }

          if( j == 0 || i == 0 || j == ln-1 || i == lm-1 ) {
            return 4;
          }

          return 5;
        })
        .setOutDep([&](int3 ijk) {
          int i = ijk[0];
          int j = ijk[1];
          int k = ijk[2];
          if(k < iter-1) {
            context.signal(stencil, {i, j, k+1});
            map<int,vector<int2>> to_fulfill; // rank -> stencil[i,j,k] on that rank at (i,j)
            int r;
            if(j == 0 && i == 0) {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            } else if (j == 0 && i == lm-1) {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
            } else if (j == ln-1 && i == 0) {
              r = block2rank({i,j-1});
              to_fulfill[r] = {{i,j-1}};
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            } else if (j == ln-1 && i == lm-1) {
              r = block2rank({i,j-1});
              to_fulfill[r] = {{i,j-1}};
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
            } else if (j == 0) {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            } else if (i == 0) {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i,j-1});
              to_fulfill[r].push_back({i,j-1});
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            } else if (j == ln-1) {
              r = block2rank({i,j-1});
              to_fulfill[r] = {{i,j-1}};
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            } else if (i == lm-1) {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i,j-1});
              to_fulfill[r].push_back({i,j-1});
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
            } else {
              r = block2rank({i,j+1});
              to_fulfill[r] = {{i,j+1}};
              r = block2rank({i,j-1});
              to_fulfill[r].push_back({i,j-1});
              r = block2rank({i-1,j});
              to_fulfill[r].push_back({i-1,j});
              r = block2rank({i+1,j});
              to_fulfill[r].push_back({i+1,j});
            }

            for(auto& p: to_fulfill) {
              r = p.first;
              // Task is local. Just fulfill.
              if(r == rank) {
                for(auto& ij: p.second) {
                  int gi = ij[0];
                  int gj = ij[1];
                  int gk = k+1;
                  context.signal(stencil, {gi,gj,gk});
                }
              // Task is remote. Send data and fulfill.
              } else {
                DTYPE * temp_A = (DTYPE *)malloc((mb*nb) * sizeof(DTYPE));

                int thread_i_start = i * mb;
                int thread_i_end = (i+1) * mb;
                int thread_j_start = j * nb;
                int thread_j_end = (j+1) * nb;

                int jj = 0;
                int ii = 0;
                for(int thread_j = thread_j_start; thread_j < thread_j_end; thread_j++) {
                  for(int thread_i = thread_i_start; thread_i < thread_i_end; thread_i++) {
                    temp_A[jj*mb+ii] = IN_2D(thread_i, thread_j, k);
                    ii++;
                  }
                  jj++;
                }

                auto Ljjv = view<double>(temp_A, mb * nb);
                auto ijsv = view<int2>(p.second.data(), p.second.size());
                am_stencil.send(r, Ljjv, i, j, k, ijsv);
                free(temp_A);
              }
            }
          } else {
            context.signalTerm();
          }

        })
        .setName([](int3 ijk) {
          return string("stencil at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
        });


    comm.barrier();
    double t0 = getWallTime();
    context.start(block_num);
    if (rank == 0){
      for(int j = 0; j < ln; j++) {
        for(int i = 0; i < lm; i++) {
          context.signal(stencil, {i, j, 1});
        }
      }
    }
    while (!context.tryJoin()) {
      comm.progress();
    }
    comm.drain();
    comm.barrier();
    double t1 = getWallTime();

    for(int j = 0; j < N; j++){
      for(int i = 0; i < M; i++){
        cout << A[iter-1][(j)*M+i] << " ";
      }
      cout << endl;
    }

    if(rank == 0) {
      printf("stencil_1D_dist nranks: %d [%dx%d] nthreads: %d M: %d mb: %d time: %.2lf\n",
             n_ranks, p, q, n_threads, M, mb, t1-t0);
    }

    for(int i = 0; i < iter; i++) {
      free(A[i]);
    }
    free(A);
  }
}


int main(int argc, char **argv)
{
  if (argc >= 2)
  {
    n_threads_ = atoi(argv[1]);
  }

  if (argc >= 3)
  {
    M_ = atoi(argv[2]); // width of matrix
  }

  if (argc >= 4)
  {
    N_ = atoi(argv[3]); // height of matrix
  }

  if (argc >= 5)
  {
    mb_ = atoi(argv[4]); // width of block
  }

  if (argc >= 6)
  {
    nb_ = atoi(argv[5]); // height of block
  }

  if (argc >= 7)
  {
    p_ = atoi(argv[6]); 
  }

  if (argc >= 8)
  {
    q_ = atoi(argv[7]);
  }

  if (argc >= 9)
  {
    VERB = atoi(argv[8]);
  }

  int n_threads = n_threads_;
  int M = M_;
  int N = N_;
  int mb = mb_;
  int nb = nb_;
  int p = p_;
  int q = q_;

  stencil_1D(n_threads, M, N, mb, nb, p, q);

  return 0;
}