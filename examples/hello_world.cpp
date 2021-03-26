#include <iostream>
#include <array>
#include "tf.hpp"

using namespace std;

using int2 = array<int, 2>;
const int m = 10;
const int n = 10;

int main() {
  tf::Context context(3);

  tf::Taskflow<int2> helloWorld;
  helloWorld.set_task([](int2 k) {
    printf("Hello world from task (%d, %d)\n", k[0], k[1]);
  })
      .set_dependency([](int2 k) {
        return 1;
      })
      .set_fulfill([&](int2 k) {
        if (k[1] < n - 1)
          context.signal(helloWorld, {k[0], k[1]+1});
      })
      .set_name([](int2 k) {
        return string("helloWorld") + to_string(k[0]) + to_string(k[1]);
      });

  // signal the first task
  for (int i = 0; i < n; ++i)
    context.signal(helloWorld, {i, 0});

  context.start();
  context.join();

  return 0;
}