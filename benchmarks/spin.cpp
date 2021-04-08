#include "tf.hpp"
#include "benchUtils.hpp"

using namespace std;

const double totalWork = 2.0;
int nthreads = 3;
double spinTime = 1e-6;
//bool includeTaskInsert = true;

int main(int argc, char **argv) {
  if (argc >= 2) {
      nthreads = atoi(argv[1]);
      if(nthreads <= 0) { printf("Wrong argument\n"); exit(1); }
  }
  if (argc >= 3) {
      spinTime = atof(argv[2]);
      if(spinTime < 0) { printf("Wrong argument\n"); exit(1); }
  }
//  if (argc >= 4) {
//      includeTaskInsert = atoi(argv[3]);
//  }
  // spin for roughly 2 seconds in total
  long ntasks = totalWork / spinTime * nthreads;
  Clock t0, t1;

  tf::Context context(nthreads);
  tf::TaskClass<int> helloWorld;
  helloWorld
      .setTask([&](int k) {
        spinForSeconds(spinTime);
      })
      .setInDep([](int k) { return 1; });
  t0 = getWallTime();
  context.start();
  for (int i = 0; i < ntasks; ++i)
    context.signal(helloWorld, i);
  context.join();
  t1 = getWallTime();
  double time = wtime_elapsed(t0, t1);
  double efficiency = totalWork / time;
  printf("nthreads: %-10d spinTime(us): %-10.3lf ntasks: %-10ld time(s): %-10.3lf efficiency %-10.3lf\n",
         nthreads, spinTime*1e6, ntasks, time, efficiency);
  return 0;
}