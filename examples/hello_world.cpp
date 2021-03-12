#include <iostream>
#include "tf.hpp"

using namespace std;

const int ntasks = 10;

int main() {
  tf::Context context(3);

  tf::Taskflow helloWorld;
  helloWorld.set_task([](int k) {
    cout << "Hello world from task " << k << "!\n";
  })
      .set_dependency([](int k) {
        return 1;
      })
      .set_fulfill([&](int k) {
        if (k < ntasks - 1)
          context.signal(helloWorld, k+1);
      })
      .set_name([](int k) {
        return string("helloWorld_") + to_string(k);
      });

  // signal the first task
  context.signal(helloWorld, 0);

  context.start();
  context.join();

  return 0;
}