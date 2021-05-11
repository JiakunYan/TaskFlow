#include "bench_common.h"
#include "tf.hpp"

using namespace std;

using int2 = std::array<int, 2>;
const double totalWork = 1.0;
int nthreads = 4;
double spinTime = 10; // in us
int nrows = nthreads * 2;
int ndeps = 3;
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
  if (argc >= 4) {
    nrows = atof(argv[3]);
    if(nrows < 1) { printf("Wrong argument\n"); exit(1); }
  } else {
    nrows = nthreads * 2;
  }
  if (argc >= 5) {
    ndeps = atof(argv[4]);
    if(ndeps < 1) { printf("Wrong argument\n"); exit(1); }
  }
  // spin for roughly 2 seconds in total
  long ntasks = totalWork / spinTime * 1e6 * nthreads;
  long ncols = (ntasks + nrows - 1) / nrows;
  assert(nrows >= ndeps);
  double t0, t1;

  tf::Context context(nthreads);
  tf::TaskClass<int2> tasks;
  tasks.setTask([&](int2 k) {
        spinForSeconds(spinTime*1e-6);
      })
      .setInDep([](int2 k) {
        int col = k[1];
        if (col == 0) return 1;
        else return ndeps;
      })
      .setOutDep([&](int2 k){
        int row = k[0];
        int col = k[1];
        if (col == ncols - 1) return;
        for (int i = 0; i < ndeps; ++i) {
          context.signal(tasks, {(row+i) % nrows, col+1});
        }
      });
  for (int i = 0; i < nrows; ++i)
    context.signal(tasks, {i, 0});
  t0 = getWallTime();
  context.start();
  context.join();
  t1 = getWallTime();
  double time = t1 - t0;
  double efficiency = (nrows * ncols * spinTime * 1e-6) / (time * nthreads);
  printf("TaskFlow nthreads: %d spinTime(us): %.3lf nrows: %d ncols: %ld ndeps: %d time(s): %.3lf efficiency %.3lf\n",
         nthreads, spinTime, nrows, ncols, ndeps, time, efficiency);
  return 0;
}