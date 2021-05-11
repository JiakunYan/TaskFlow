#include <iostream>
#include <array>
#include "tf.hpp"

using namespace std;

int main() {
  tf::Context context(1);
  tf::Communicator comm;

  volatile bool flag = false;

  function<void(void)> fn = [&]() {
    flag = true;
  };
  auto am = comm.makeActiveMsg(fn);

  if (comm.rank_me() == 0) {
    for (int i = 0; i < comm.rank_n(); ++i)
      am.send(i);
  }
  while (!flag) {
    comm.progress();
  }
  comm.drain();
  comm.barrier();
  return 0;
}