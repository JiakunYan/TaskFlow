#include <iostream>
#include <array>
#include <ctime>
#include <ratio>
#include <chrono>
#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <map>
#include "tf.hpp"
#include "benchUtils.hpp"

using namespace std;
using namespace Eigen;

typedef array<int, 2> int2;
typedef array<int, 3> int3;

// Declare global variables for current program
int n_threads = 4;
double spinTime = 1e-5;
int N = 160;
int n = 10;
int p = 1;
int q = 1;

//bool includeTaskInsert = true;

int main(int argc, char **argv) {

    // parse input arguments
    if (argc >= 2) {
        n_threads = atoi(argv[1]);
        if(n_threads <= 0) { printf("Wrong argument\n"); exit(1); }
    }
    if (argc >= 3) {
        N = atof(argv[2]);
        if(N < 1) { printf("Wrong argument\n"); exit(1); }
    }
    if (argc >= 4) {
        n = atof(argv[3]);
        if(n < 1) { printf("Wrong argument\n"); exit(1); }
    }

    Clock t0, t1;

    // Set maximum concurrency for current work
    tf::Context context(n_threads);

    // Create three task classes to be ran
    tf::TaskClass<int> potf_tf;
    tf::TaskClass<int2> trsm_tf;
    tf::TaskClass<int3> gemm_tf;

    
    // Create an matrix A
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

    // WiKi example
    //  MatrixXd A(N, N);
    //  A << 4, 12, -16,
    //       12, 37, -43,
    //       -16, -43, 98;
    //  MatrixXd Aref = A;

  
    IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");
    //  cout << "Input Matrix A:" << endl;
    //  cout << A.format(CleanFmt) << endl;
    //  cout << endl;

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

        double current_prio = static_cast<double>(N*N - ((i*(i-1))/2 + j));

        int priority = 0;
        if (current_prio < 0.1 * N * N) {
            priority = 0;
        } else if (current_prio < 0.3 * N * N) {
            priority = 1;
        } else if (current_prio > 0.5 * N * N) {
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
        for(int j = 0; j <= i; j++) {
            if(block2rank({i,j}) == 0) {
                Mat[{i,j}] = A.block(i * n, j * n, n, n);
            } else {
                Mat[{i,j}] = MatrixXd::Zero(n, n);
            }
        }
    }

    // Create POTF tasks and declare dependencies 
    potf_tf.setTask([&](int j) {
        LLT<Ref<MatrixXd>> llt(Mat.at({j, j}));
        assert(llt.info() == Eigen::Success);
    })
      .setInDep([](int) {
        return 1;
      })
      .setOutDep([&](int j) {
        /*
          potrf(j) should signal 
          trsm(j+1, j) across trsm(N, j)
        */
        for (int i = j + 1; i < N; i++) {
          context.signal(trsm_tf, {i, j});
        }
        
      })
      .setName([](int j) {
        return string("POTF at ") + to_string(j);
      })
      .setPriority([&](int j) {
        return block2prio({j, j});
      });


    // Create TRSM tasks and declare dependencies
    trsm_tf.setTask([&](int2 ij) {
        int i = ij[0];
        int j = ij[1];
        auto L = Mat.at({j, j}).triangularView<Lower>().transpose();
        Mat.at({i, j}) = L.solve<OnTheRight>(Mat.at({i, j}));
    })
      .setInDep([](int2 ij) {
        return (ij[1] == 0 ? 1 : 2);
      })
      .setOutDep([&](int2 ij) {
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
      .setName([](int2 ij) {
        return string("TRSM at ") + to_string(ij[0]) + "_" + to_string(ij[1]);
      })
      .setPriority([&](int2 ij) {
        return block2prio(ij);
      });
  

    // Create GEMM tasks and declare dependencies
    gemm_tf.setTask([&](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];
        Mat.at({i,j}).noalias() -= Mat.at({i,k}) * Mat.at({j,k}).transpose();
    })
      .setInDep([](int3 ijk) {
        int i = ijk[0];
        int j = ijk[1];
        int k = ijk[2];
        return (i == j ? 1 : 2) + (k > 0 ? 1 : 0);
      })
      .setOutDep([&](int3 ijk) {
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
      .setName([](int3 ijk) {
        return string("GEMM at ") + to_string(ijk[0]) + "_" + to_string(ijk[1]) + "_" + to_string(ijk[2]);
      })
      .setPriority([&](int3 ijk) {
        return block2prio({ijk[0], ijk[1]});
      });
  

    context.signal(potf_tf, 0);
    t0 = getWallTime();
    
    context.start();
    context.join();

    t1 = getWallTime();
    double time = wtime_elapsed(t0, t1);

    //double efficiency = ( * ncols * spinTime) / (time * nthreads);
    printf("nthreads: %-10d N: %-10d n: %-10ld time(s): %-10.3lf \n",
            n_threads, N, n, time);
    return 0;
}