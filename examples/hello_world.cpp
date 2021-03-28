#include <iostream>
#include <array>
#include "tf.hpp"

using namespace std;

using int2 = array<int, 2>;
const int m = 5;
const int n = 10;
int main() {
  tf::Context context(3);
  // define tasks
  tf::TaskClass<int2> helloWorld;
  helloWorld
      .setTask([&](int2 k) {
        printf("Hello world from task (%d, %d) of xstream %d/%d\n",
               k[0], k[1], context.rank_me(), context.rank_n());
      })
      .setInDep([](int2 k) { return 1; })
      .setOutDep([&](int2 k) {
        if (k[1] < n - 1)
          context.signal(helloWorld, {k[0], k[1] + 1});
      })
      .setName([](int2 k) {
        return string("helloWorld") + to_string(k[0]) + to_string(k[1]);
      });
  // signal initial tasks
  for (int i = 0; i < m; ++i)
    context.signal(helloWorld, {i, 0});
  // execute tasks
  context.start();
  context.join();
  return 0;
}