#include <iostream>
#include <array>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include "tf.hpp"
#include "bench_common.h"

using namespace std;

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


void stencil_1D(int n_threads, int M, int N, int mb, int nb){
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

  // define tasks
  tf::TaskClass<int3> stencil;

  stencil.setTask([&](int3 ijk) {
    int i = ijk[0];
    int j = ijk[1];
    int k = ijk[2];

    int thread_i_start = i * mb;
    int thread_i_end = (i+1) * mb;
    int thread_j_start = j * nb;
    int thread_j_end = (j+1) * nb;;

    // printf("%d %d %d: %d and %d\n", ijk[0], ijk[1], ijk[2], thread_i_start, thread_j_start);
    // printf("%d %d %d: %d and %d\n", ijk[0], ijk[1], ijk[2], thread_i_end, thread_j_end);

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
          if(j == 0 && i == 0) {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          } else if (j == 0 && i == lm-1) {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
          } else if (j == ln-1 && i == 0) {
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          } else if (j == ln-1 && i == lm-1) {
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
          } else if (j == 0) {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          } else if (i == 0) {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          } else if (j == ln-1) {
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          } else if (i == lm-1) {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
          } else {
            context.signal(stencil, {i, j+1, k+1}); 
            context.signal(stencil, {i, j-1, k+1}); 
            context.signal(stencil, {i-1, j, k+1}); 
            context.signal(stencil, {i+1, j, k+1}); 
          }
        }
      })
      .setName([](int3 ijk) {
        return string("stencil at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
      });

  // signal initial tasks
  for(int j = 0; j < ln; j++) {
    for(int i = 0; i < lm; i++) {
      context.signal(stencil, {i, j, 1});
    }
  }

  auto start = chrono::high_resolution_clock::now();

  // Start the context
  context.start();
  context.join();
  auto end = chrono::high_resolution_clock::now();

  // for(int j = 0; j < N; j++){
  //   for(int i = 0; i < M; i++){
  //     cout << A[iter-1][(j)*M+i] << " ";
  //   }
  //   cout << endl;
  // }

  chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(end - start);
  printf("%lf\n", time_span.count());

  for(int i = 0; i < iter; i++) {
    free(A[i]);
  }
  free(A);

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
    nb_ = atoi(argv[5]);
  }

  if (argc >= 7)
  {
    VERB = atoi(argv[6]);
  }

  int n_threads = n_threads_;
  int M = M_;
  int N = N_;
  int mb = mb_;
  int nb = nb_;

  stencil_1D(n_threads, M, N, mb, nb);

  return 0;
}