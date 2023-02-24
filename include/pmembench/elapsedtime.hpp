#pragma once

#include <chrono>

namespace pmembench {
  class ElapsedTime {
    std::chrono::high_resolution_clock::time_point t0_;
    std::chrono::high_resolution_clock::time_point t1_;
    bool freezed_ = false;

  public:
    ElapsedTime() { reset(); }

    void reset() {
      t0_ = std::chrono::high_resolution_clock::now();
      freezed_ = false;
    }

    void freeze() {
      t1_ = std::chrono::high_resolution_clock::now();
      freezed_ = true;
    }

    auto sec() -> double {
      const auto end = freezed_ ? t1_ : std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<std::chrono::duration<double>>(end - t0_).count();
    }

    auto msec() -> double { return this->sec() * 1000; }
  };

  class ElapsedTimeRaii {
    ElapsedTime& t_;

  public:
    ElapsedTimeRaii(ElapsedTime& t) : t_(t) { t_.reset(); }
    ~ElapsedTimeRaii() { t_.freeze(); }
  };
}  // namespace pmembench
