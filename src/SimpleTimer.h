#ifndef __SIMPLETIMER_H_
#define __SIMPLETIMER_H_

#include <chrono>

namespace only_for_benchmark {
struct SimpleTimer {
  inline void restart();
  inline double elapsed();
 private:
  std::chrono::high_resolution_clock::time_point _start;
};

using namespace std::chrono;

void SimpleTimer::restart() { _start = high_resolution_clock::now(); }

double SimpleTimer::elapsed() {
  auto cur = high_resolution_clock::now();
  duration<double, std::micro> fp_us = cur - _start;
  return fp_us.count();
}
} // only_for_benchmark
#endif // __SIMPLETIMER_H_