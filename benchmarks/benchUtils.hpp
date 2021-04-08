#ifndef TASKFLOW_BENCHUTILS_HPP
#define TASKFLOW_BENCHUTILS_HPP

#include <chrono>

using Clock = std::chrono::time_point<std::chrono::high_resolution_clock>;
std::chrono::time_point<std::chrono::high_resolution_clock> getWallTime() {
  return std::chrono::high_resolution_clock::now();
};

double wtime_elapsed(const std::chrono::time_point<std::chrono::high_resolution_clock>& t0, const std::chrono::time_point<std::chrono::high_resolution_clock>& t1) {
  return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
};

void spinForSeconds(double time) {
    auto t0 = std::chrono::high_resolution_clock::now();
    while(true) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
        if( elapsed_time >= time ) break;
    }
}

#endif // TASKFLOW_BENCHUTILS_HPP
