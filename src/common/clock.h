#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace mx {

inline std::int64_t now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

class LatencyRecorder {
 public:
  explicit LatencyRecorder(std::size_t reserve = 1 << 20) { samples_.reserve(reserve); }

  void record(std::int64_t ns) { samples_.push_back(ns); }
  std::size_t count() const { return samples_.size(); }

  struct Summary {
    std::size_t n = 0;
    std::int64_t min = 0, p50 = 0, p90 = 0, p99 = 0, p999 = 0, max = 0;
    double mean = 0;
  };

  Summary summary() {
    Summary s;
    if (samples_.empty()) return s;
    std::sort(samples_.begin(), samples_.end());
    auto pct = [&](double p) {
      auto idx = static_cast<std::size_t>(p * static_cast<double>(samples_.size() - 1));
      return samples_[idx];
    };
    double total = 0;
    for (auto v : samples_) total += static_cast<double>(v);
    s.n = samples_.size();
    s.min = samples_.front();
    s.p50 = pct(0.50);
    s.p90 = pct(0.90);
    s.p99 = pct(0.99);
    s.p999 = pct(0.999);
    s.max = samples_.back();
    s.mean = total / static_cast<double>(s.n);
    return s;
  }

  static std::string format(const Summary& s, const char* unit = "ns", double div = 1.0) {
    auto as_d = [](std::int64_t v) { return static_cast<double>(v); };
    char buf[256];
    std::snprintf(buf, sizeof buf,
                  "n=%zu  min=%.1f  p50=%.1f  p90=%.1f  p99=%.1f  p99.9=%.1f  max=%.1f  mean=%.1f (%s)", s.n,
                  as_d(s.min) / div, as_d(s.p50) / div, as_d(s.p90) / div, as_d(s.p99) / div,
                  as_d(s.p999) / div, as_d(s.max) / div, s.mean / div, unit);
    return buf;
  }

 private:
  std::vector<std::int64_t> samples_;
};

}
