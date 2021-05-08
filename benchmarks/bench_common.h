#ifndef TASKFLOW_BENCH_COMMON_H
#define TASKFLOW_BENCH_COMMON_H

#include <sys/time.h>

static inline double getWallTime() {
  struct timeval t1;
  gettimeofday(&t1,0);
  return t1.tv_sec + t1.tv_usec / 1e6;
};

void spinForSeconds(double time) {
    double t0 = getWallTime();
    while(true) {
        double t1 = getWallTime();
        double elapsed_time = t1 - t0;
        if( elapsed_time >= time ) break;
    }
}

#endif // TASKFLOW_BENCH_COMMON_H
