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
#define WEIGHT_1D(jj) weight_1D[jj+R]
#define IN_2D(i,j,k)  A[k][j*M+i]
#define OUT_2D(i,j,k) A[k][j*N+i]

// Global Constant
const int n_threads = 12;

int main() {
  /* Default */
  // int m = 0;
  int M = 10;
  int N = 10;
  int mb = 1; // width of block
  int nb = 1; // height of block
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

  for(int k = 1; k < iter; k++) {
    for(int thread_j = 0; thread_j < N; thread_j++) {
      for(int thread_i = 0; thread_i < M; thread_i++) {
        //printf("%d and %d\n", thread_j, thread_i); 
        if(thread_j >= R && thread_j < N - R) {
          OUT_2D(thread_i, thread_j, k) = WEIGHT_1D(-R) * IN_2D(thread_i, thread_j-R, k-1);
          for (int jj = -R+1; jj <= R; jj++) {
              OUT_2D(thread_i, thread_j, k) += WEIGHT_1D(jj) * IN_2D(thread_i, thread_j+jj, k-1);
          }
        }
      }
    }
  }

  for(int j = 0; j < N; j++){
    for(int i = 0; i < M; i++){
      cout << A[iter-1][(j)*M+i] << " ";
    }
    cout << endl;
  }

  for(int i = 0; i < iter; i++) {
    free(A[i]);
  }
  free(A);

  return 0;
}