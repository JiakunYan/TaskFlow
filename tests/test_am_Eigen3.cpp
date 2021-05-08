#include <iostream>
#include <array>
#include <Eigen/Core>
#include "tf.hpp"

using namespace std;
using namespace Eigen;

const int nrows = 100;
const int ncols = 100;

int main() {
  tf::Context context(1);
  tf::Communicator comm;
  assert(comm.rank_n() == 2);

  MatrixXd m;
  volatile bool flag = false;

  function<void(tf::view<double>&)> fn = [&](tf::view<double>& m_) {
    m = Map<MatrixXd>(m_.data(), nrows, ncols);
    flag = true;
  };
  auto am = comm.makeActiveMsg(fn);

  if (comm.rank_me() == 0) {
    MatrixXd mm = MatrixXd::Random(nrows, ncols);
//    cout << "rank 0 send:\n" << mm << endl;
    am.send(1, tf::view<double>(mm.data(), mm.size()));
    while (!flag) {
      comm.progress();
    }
    double error = (mm - m).norm() / mm.norm();
    printf("Am transmission error: %lf\n", error);
  } else {
    while (!flag) {
      comm.progress();
    }
    am.send(0, tf::view<double>(m.data(), m.size()));
//    cout << "rank 1 recv:\n" << m << endl;
  }
  comm.drain();
  comm.barrier();
  return 0;
}