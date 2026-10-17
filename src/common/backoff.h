#pragma once

#include <chrono>
#include <thread>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

namespace mx {

inline void cpu_relax() {
#if defined(__x86_64__) || defined(__i386__)
  _mm_pause();
#elif defined(__aarch64__)
  asm volatile("yield");
#endif
}

class Backoff {
 public:
  explicit Backoff(bool busy_spin = false) : busy_spin_(busy_spin) {}

  void reset() { idle_rounds_ = 0; }

  void idle() {
    if (busy_spin_) {
      cpu_relax();
      return;
    }
    ++idle_rounds_;
    if (idle_rounds_ < 200) {
      cpu_relax();
    } else if (idle_rounds_ < 400) {
      std::this_thread::yield();
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
  }

 private:
  bool busy_spin_;
  unsigned idle_rounds_ = 0;
};

}
