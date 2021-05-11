#include <iostream>
#include <array>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tf.hpp"

using namespace std;

using int2 = array<int, 2>;

#define DTYPE double
#define WEIGHT_1D(jj) weight_1D[jj+R]
#define IN_2D(i,j)    A[(j)*mb+i]
#define OUT_2D(i,j)   out_A[(j)*mb+i]

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
  DTYPE *A = (DTYPE *)malloc(sizeof(DTYPE) * (mb*nb));
  DTYPE *out_A = (DTYPE *)malloc(sizeof(DTYPE) * (mb*nb));

  for(int j = R; j < nb - R; j++)
      for(int i = 0; i < mb; i++)
          A[j*mb+i] = (DTYPE)1.0 * i + (DTYPE)1.0 * j;

  for(int j = 0; j < R; j++)
      for(int i = 0; i < mb; i++)
          A[j*mb+i] = (DTYPE)0.0;

  for(int j = nb - R; j < nb; j++)
      for(int i = 0; i < mb; i++)
          A[j*mb+i] = (DTYPE)0.0;

  for(int j = 0; j < nb; j++){
    for(int i = 0; i < mb; i++){
      cout << A[(j)*mb+i] << " ";
    }
    cout << endl;
  }


  tf::Context context(n_threads);

  // define tasks
  tf::TaskClass<int2> stencil;

  stencil.setTask([&](int2 ij) {
    int i = ij[0];
    int j = ij[1];
    OUT_2D(i, j) = WEIGHT_1D(-R) * IN_2D(i, j-R);
    for (int jj = -R+1; jj <= R; jj++) {
        OUT_2D(i, j) += WEIGHT_1D(jj) * IN_2D(i, j+jj);
    }
  })
      .setInDep([](int2 ij) {
        return 1;
        //return (ij[1] == 0 ? 1 : 2);
      })
      .setOutDep([&](int2 ij) {

      })
      .setName([](int2 ij) {
        return string("stencil at ") + to_string(ij[0]) + "_" + to_string(ij[1]);
      });

  // signal initial tasks
  for(int j = R; j < nb-R; j++) {
    for(int i = 0; i < mb; i++) {
      context.signal(stencil, {i, j});
    }
  }

  // execute tasks
  context.start();
  context.join();

  for(int j = 0; j < nb; j++){
    for(int i = 0; i < mb; i++){
      cout << out_A[(j)*mb+i] << " ";
    }
    cout << endl;
  }

  return 0;
}