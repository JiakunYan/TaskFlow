#include <iostream>
#include <array>
#include "tf.hpp"

using namespace std;

const int nmsgs = 10;

int main() {
  tf::Context context(3);

  tf::Communicator comm;

  int msg_recved = 0;
  function<void(int)> fn = [&](int source) {
    printf("hello world from rank %d to rank %d/%d\n", source, comm.rank_me(),
           comm.rank_n());
    ++msg_recved;
  };
  auto am = comm.makeActiveMsg(fn);
  for (int i = 0; i < nmsgs * comm.rank_n(); ++i) {
    int target = i % comm.rank_n();
    am.send(target, comm.rank_me());
  }

  while (msg_recved < comm.rank_n() * nmsgs) {
    comm.progress();
  }
  return 0;
}