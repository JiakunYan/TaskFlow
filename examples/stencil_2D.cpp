#include <iostream>
#include <array>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include "tf.hpp"

using namespace std;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

#define DTYPE double
#define WEIGHT_2D(i, jj) weight_2D[i][jj+R]
#define IN_2D(i,j,k)  A[k][j*M+i]
#define OUT_2D(i,j,k) A[k][j*N+i]

// Global Constant
const int n_threads = 12;

int main() {
  /* Default */
  // int m = 0;
  int M = 1000;
  int N = 1000;
  int mb = 10; // width of block
  int nb = 10; // height of block
  int lm = M/mb;
  int ln = N/nb;
  int block_num = lm * ln;
  // int P = 1;
  // int KP = 1;
  // int KQ = 1;
  // int cores = -1;
  int iter = 10;
  int R = 1;
  int block_count[iter];

  /* intialize weight_2D */
  DTYPE **weight_2D = (DTYPE **)malloc(sizeof(DTYPE *) * (2*R+1));
  for(int i = 0; i < 2*R+1; i++) {
    weight_2D[i] = (DTYPE *)malloc(sizeof(DTYPE) * (2*R+1));
  }

  for(int i = 0; i < 2*R+1; i++) {
    for(int jj = 1; jj <= R; jj++) {
      WEIGHT_2D(i,jj) = (DTYPE)(1.0/(2.0*jj*R));
      WEIGHT_2D(i,-jj) = -(DTYPE)(1.0/(2.0*jj*R));
    }
    WEIGHT_2D(i,0) = (DTYPE)1.0;
  }
  

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
        if(thread_j >= R && thread_j < N - R && thread_i >= R && thread_i < M - R) {
          OUT_2D(thread_i, thread_j, k) = (DTYPE)0.0;
          for (int ii = 0; ii < 2*R+1; ii++) {
            for (int jj = -R; jj <= R; jj++) {
                OUT_2D(thread_i, thread_j, k) += WEIGHT_2D(ii,jj) * IN_2D(thread_i+ii-R, thread_j+jj, k-1);
            }
          }
        }
      }
    }

    // block_count[k] += 1;
    // if(k != iter-1 && block_count[k] == block_num){
    //   free(A[k]);
    // }

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
  // free(A[iter-1]);
  free(A);

  return 0;
}