#include <iostream>
#include <array>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Cholesky>
#include <map>
#include "tf.hpp"

using namespace std;
using namespace Eigen;


typedef array<int, 2> int2;
typedef array<int, 3> int3;

// Global Constant
const int n_threads = 3;

int main() {
  
  // Set maximum concurrency for current work
  tf::Context context(n_threads);

  // Create three taskflows to be ran
  tf::Taskflow<int> potf_tf;
  tf::Taskflow<int2> trsm_tf;
  tf::Taskflow<int3> gemm_tf;

  // Set global variabls
  int N = 3;
  int n = 1;
  

  int p = 1;
  int q = 1;

  // Create an matrix A
  // Form the matrix : let every node have a copy of A for now
  /*
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
  //A = A.triangularView<Lower>();
  */
 
  // WiKi example
  MatrixXd A(N, N);
  A << 4, 12, -16,
       12, 37, -43,
       -16, -43, 98;

  
  IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");
  cout << "Input Matrix A:" << endl;
  cout << A.format(CleanFmt) << endl;
  cout << endl;

  // Mapper
  auto block2rank = [&](int2 ij){
      int i = ij[0];
      int j = ij[1];
      int ii = i % p;
      int jj = j % q;
      int r = ii + jj * p;
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
          if(block2rank({i,j}) == 0) {
            Mat[{i,j}] = A.block(i * n, j * n, n, n);
          } else {
            Mat[{i,j}] = MatrixXd::Zero(n, n);
          }
      }
  }

  // Create POTF tasks and declare dependencies 
  potf_tf.set_task([&](int j) {
    LLT<Ref<MatrixXd>> llt(Mat.at({j, j}));
    assert(llt.info() == Eigen::Success);
  })
      .set_dependency([](int) {
        return 1;
      })
      .set_fulfill([&](int j) {
        /*
          potrf(j) should signal 
          trsm(j+1, j) across trsm(N, j)
        */
        for (int i = j + 1; i < N; i++) {
          context.signal(trsm_tf, {i, j});
        }
        
      })
      .set_name([](int j) {
        return string("POTF at ") + to_string(j);
      });


  // Create TRSM tasks and declare dependencies
  trsm_tf.set_task([&](int2 ij) {
    int i = ij[0];
    int j = ij[1];
    auto L = Mat.at({j, j}).triangularView<Lower>().transpose();
    Mat.at({i, j}) = L.solve<OnTheRight>(Mat.at({i, j}));
  })
      .set_dependency([](int2 ij) {
        return (ij[1] == 0 ? 1 : 2);
      })
      .set_fulfill([&](int2 ij) {
        /* 
          trsm(i, j) should signal
          gemm(i, j+1, j) across gemm(i, i, j)
          and
          gemm(i+1, i, j) across gemm(N, i, j)
        
        */

        int i = ij[0];
        int j = ij[1];

        int gemm_i;
        int gemm_j;
        int gemm_k = j; //The same as "j"

        gemm_i = i;
        for (gemm_j = j + 1; gemm_j <= i; gemm_j++) {
          context.signal(gemm_tf, {gemm_i, gemm_j, gemm_k});
        }

        gemm_j = i;
        for (gemm_i = i + 1; gemm_i < N; gemm_i++) {
          context.signal(gemm_tf, {gemm_i, gemm_j, gemm_k});
        }
        
      })
      .set_name([](int2 ij) {
        return string("TRSM at ") + to_string(ij[0]) + "_" + to_string(ij[1]);
      });
  

  // Create GEMM tasks and declare dependencies
  gemm_tf.set_task([&](int3 ijk) {
    int i = ijk[0];
    int j = ijk[1];
    int k = ijk[2];
    Mat.at({i,j}).noalias() -= Mat.at({i,k}) * Mat.at({j,k}).transpose();
  })
      .set_dependency([](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];
        return (i == j ? 1 : 2) + (k > 0 ? 1 : 0);
      })
      .set_fulfill([&](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];

        if (k + 1 == i && k + 1 == j) {
          context.signal(potf_tf, k+1); 
        } else if (k + 1 == j) {
          context.signal(trsm_tf, {i, k + 1}); 
        } else {
          context.signal(gemm_tf, {i, j, k + 1}); 
        }
        
      })
      .set_name([](int3 ijk) {
        return string("GEMM at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
      });
  
  // signal the first task
  context.signal(potf_tf, 0);
  

  context.start();
  context.join();

  cout << "Solved L from the Cholesky decomposition: " << endl;
  for (int i = 0; i < N; i++) {
    cout << "[ ";
    for (int j = 0; j <= i; j++) {
      cout << Mat.at({i, j}) << " ";
    }
    for (int k = 0; k < N - 1 - i; k++) {
      cout << "0" << " ";
    }
    
    cout << "]"  << endl;
  }
  
  cout << endl;

  return 0;
}