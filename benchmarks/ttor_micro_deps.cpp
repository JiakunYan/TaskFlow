#include "bench_common.h"
#include "tasktorrent.hpp"

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
  double t0, t1;

  ttor::Threadpool_shared tp(nthreads, 0, "Wk_", false);
  ttor::Taskflow<int2> tasks(&tp, 0);
  tasks.set_task([&](int2 k) {
        spinForSeconds(spinTime);
      })
      .set_indegree([](int2 k) {
        int col = k[1];
        if (col == 0) return 1;
        else return ndeps;
      })
      .set_fulfill([&](int2 k){
        int row = k[0];
        int col = k[1];
        if (col == ncols - 1) return;
        for (int i = 0; i < ndeps; ++i) {
          tasks.fulfill_promise({(row+i) % nrows, col+1});
        }
      })
      .set_mapping([&](int2 k) {
        return (k[0] % nthreads);
      });
  t0 = getWallTime();
  tp.start();
  for (int i = 0; i < nrows; ++i)
    tasks.fulfill_promise({i, 0});
  tp.join();
  t1 = getWallTime();
  double time = t1 - t0;
  double efficiency = (nrows * ncols * spinTime) / (time * nthreads);
  printf("nthreads: %-10d spinTime(us): %-10.3lf nrows: %-10d ncols: %-10ld ndeps: %-10d time(s): %-10.3lf efficiency %-10.3lf\n",
         nthreads, spinTime*1e6, nrows, ncols, ndeps, time, efficiency);
  return 0;
}