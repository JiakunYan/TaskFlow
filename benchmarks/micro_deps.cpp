#include "tf.hpp"
#include "benchUtils.hpp"

using namespace std;

using int2 = std::array<int, 2>;
const double totalWork = 1.0;
int nthreads = 3;
double spinTime = 1e-5;
int nrows = nthreads;
int ndeps = 1;
//bool includeTaskInsert = true;

int main(int argc, char **argv) {
  if (argc >= 2) {
    nthreads = atoi(argv[1]);
    nrows = nthreads;
    if(nthreads <= 0) { printf("Wrong argument\n"); exit(1); }
  }
  if (argc >= 3) {
    spinTime = atof(argv[2]);
    if(spinTime < 0) { printf("Wrong argument\n"); exit(1); }
  }
  if (argc >= 4) {
    nrows = atof(argv[3]);
    if(nrows < 1) { printf("Wrong argument\n"); exit(1); }
  }
  if (argc >= 5) {
    ndeps = atof(argv[4]);
    if(ndeps < 1) { printf("Wrong argument\n"); exit(1); }
  }
  // spin for roughly 2 seconds in total
  long ntasks = totalWork / spinTime * nthreads;
  long ncols = (ntasks + nrows - 1) / nrows;
  assert(nrows >= ndeps);
  Clock t0, t1;

  tf::Context context(nthreads);
  tf::TaskClass<int2> tasks;
  tasks.setTask([&](int2 k) {
        spinForSeconds(spinTime);
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
  t0 = getWallTime();
  context.start();
  for (int i = 0; i < nrows; ++i)
    context.signal(tasks, {i, 0});
  context.join();
  t1 = getWallTime();
  double time = wtime_elapsed(t0, t1);
  double efficiency = (nrows * ncols * spinTime) / (time * nthreads);
  printf("nthreads: %-10d spinTime(us): %-10.3lf nrows: %-10d ncols: %-10ld ndeps: %-10d time(s): %-10.3lf efficiency %-10.3lf\n",
         nthreads, spinTime*1e6, nrows, ncols, ndeps, time, efficiency);
  return 0;
}