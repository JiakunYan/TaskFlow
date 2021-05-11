#include "tf.hpp"
using int2 = std::array<int, 2>;
const int m = 5, n = 10, ndeps = 3;
int main() {
  tf::Context context(3);
  // define tasks
  tf::TaskClass<int2> helloWorld;

  tf::Communicator comm;
  std::function<void(int2)> fn = [&](int2 k) {
    context.signal(helloWorld, k);
  };
  auto am = comm.makeActiveMsg(fn);

  auto idx2rank = [&](int2 k) {
    return k[0] % comm.rank_n();
  };

  helloWorld
      .setTask([&](int2 k) {
        printf("Hello world from task (%d, %d) of xstream %d/%d rank %d/%d\n",
               k[0], k[1], context.rank_me(), context.rank_n(), comm.rank_me(), comm.rank_n());
      })
      .setInDep([](int2 k) {
        return (k[1] == 0)? 1 : ndeps;
      })
      .setOutDep([&](int2 k) {
        if (k[1] < n - 1) {
          for (int i = 0; i < ndeps; ++i) {
            int2 succ = {(k[0] + i) % (m * comm.rank_n()), k[1] + 1};
            int rank = idx2rank(succ);
            if (rank == comm.rank_me())
              context.signal(helloWorld, succ);
            else
              am.send(rank, succ);
          }
        } else {
          context.signalTerm();
        }
      });

  // signal initial tasks
  for (int i = 0; i < m; ++i)
    context.signal(helloWorld, {i * comm.rank_n() + comm.rank_me(), 0});

  // execute tasks
  context.start(m);
  while (!context.tryJoin())
    comm.progress();
  comm.barrier();
  return 0;
}