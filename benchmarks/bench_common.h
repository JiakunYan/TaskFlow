#ifndef TASKFLOW_BENCH_COMMON_H
#define TASKFLOW_BENCH_COMMON_H

#include <iostream>
#include <sys/time.h>
#include <getopt.h>

#define LARGE 8192
#define TOTAL 40000
#define SKIP 10000
#define TOTAL_LARGE 10000
#define SKIP_LARGE 1000

/* Names for the values of the 'has_arg' field of 'struct option'.  */

#define no_argument		0
#define required_argument	1
#define optional_argument	2

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

void write_buffer(char *buffer, int len, char input) {
  for (int i = 0; i < len; ++i) {
    buffer[i] = input;
  }
}

void check_buffer(const char *buffer, int len, char expect) {
  for (int i = 0; i < len; ++i) {
    if (buffer[i] != expect) {
      printf("check_buffer failed! buffer[%d](%d) != %d. ABORT!\n", i, buffer[i], expect);
      abort();
    }
  }
}

static inline double get_latency(double time, double n_msg) {
  return time / n_msg;
}

static inline double get_msgrate(double time, double n_msg) {
  return n_msg / time;
}

static inline double get_bw(double time, size_t size, double n_msg) {
  return n_msg * size / time;
}

template<typename FUNC>
static inline void RUN_VARY_MSG(std::pair<size_t, size_t> &&range,
                                const int report,
                                FUNC &&f, std::pair<int, int> &&iter = {0, 1}) {
  double t = 0;
  int loop = TOTAL;
  int skip = SKIP;
  long long state;
  long long count = 0;


  for (size_t msg_size = range.first; msg_size <= range.second; msg_size <<= 1) {
    if (msg_size >= LARGE) {
      loop = TOTAL_LARGE;
      skip = SKIP_LARGE;
    }

    for (int i = iter.first; i < skip; i += iter.second) {
      f(msg_size, i);
    }

    t = getWallTime();

    for (int i = iter.first; i < loop; i += iter.second) {
      f(msg_size, i);
    }

    t = getWallTime() - t;

    if (report) {
      double latency = 1e6 * get_latency(t, 2.0 * loop); // one-way latency
      double msgrate = get_msgrate(t, loop) / 1e6;           // single-direction message rate
      double bw = get_bw(t, msg_size, loop) / 1024 / 1024;   // single-direction bandwidth

      char output_str[256];
      int used = 0;
      used += snprintf(output_str + used, 256, "%-10lu %-10.2f %-10.3f %-10.2f",
                       msg_size, latency, msgrate, bw);
      printf("%s\n", output_str);
      fflush(stdout);
    }
  }
}

#endif // TASKFLOW_BENCH_COMMON_H
