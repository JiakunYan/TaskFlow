#include <iostream>
#include <array>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tf.hpp"

using namespace std;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

#define DTYPE double
#define WEIGHT_1D(jj) weight_1D[jj+R]
#define IN_2D(i,j,k)  A[k][j*mb+i]
#define OUT_2D(i,j,k) A[k][j*mb+i]

// Global Constant
const int n_threads = 12;

int main() {
  /* Default */
  // int m = 0;
  // int M = 8;
  // int N = 8;
  int mb = 8;
  int nb = 8;
  // int P = 1;
  // int KP = 1;
  // int KQ = 1;
  // int cores = -1;
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
    A[i] = (DTYPE *)malloc((mb*nb) * sizeof(DTYPE));
  }

  for(int k = 0; k < iter; k++)
    for(int j = R; j < nb - R; j++)
        for(int i = 0; i < mb; i++)
            A[k][j*mb+i] = (DTYPE)1.0 * i + (DTYPE)1.0 * j;

  for(int k = 0; k < iter; k++)
    for(int j = 0; j < R; j++)
      for(int i = 0; i < mb; i++)
          A[k][j*mb+i] = (DTYPE)0.0;

  for(int k = 0; k < iter; k++)
    for(int j = nb - R; j < nb; j++)
        for(int i = 0; i < mb; i++)
            A[k][j*mb+i] = (DTYPE)0.0;


  tf::Context context(n_threads);

  // define tasks
  tf::TaskClass<int3> stencil;

  stencil.setTask([&](int3 ijk) {
    int i = ijk[0];
    int j = ijk[1];
    int k = ijk[2];
    // printf("Hello world from task (%d, %d, %d) of xstream\n", ijk[0], ijk[1], ijk[2]);
    OUT_2D(i, j, k) = WEIGHT_1D(-R) * IN_2D(i, j-R, k-1);
    for (int jj = -R+1; jj <= R; jj++) {
        OUT_2D(i, j, k) += WEIGHT_1D(jj) * IN_2D(i, j+jj, k-1);
    }
  })
      .setInDep([&](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];

        if(k == 1) {
          return 1;
        }

        if(j >= R && j <= 2*R-1) {
          return j+1; // j-R+R+1
        }

        if(j >= nb-2*R && j <= nb-R-1) {
          return nb-j;
        }

        return 2*R+1;
      })
      .setOutDep([&](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];
        if(k < iter-1) {
          for(int jj = -R; jj <= R; jj++) {
            context.signal(stencil, {i, j+jj, k+1}); 
          }
        }
      })
      .setName([](int3 ijk) {
        return string("stencil at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
      });

  // signal initial tasks
  for(int j = R; j < nb-R; j++) {
    for(int i = 0; i < mb; i++) {
      context.signal(stencil, {i, j, 1});
    }
  }

  // execute tasks
  context.start();
  context.join();

  for(int k = 0; k < iter; k++){
    for(int j = 0; j < nb; j++){
      for(int i = 0; i < mb; i++){
        cout << A[k][(j)*mb+i] << " ";
      }
      cout << endl;
    }
  }

  for(int i = 0; i < iter; i++) {
    free(A[i]);
  }
  free(A);

  return 0;
}