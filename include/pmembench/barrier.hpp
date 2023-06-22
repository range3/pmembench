#pragma once

#include <condition_variable>
#include <mutex>

namespace pmembench {
  class Barrier {
    size_t nthreads_;
    std::mutex mtx_;
    std::condition_variable cond_;

  public:
    Barrier(size_t nthreads) : nthreads_(nthreads) {}

    void wait() {
      {
        std::unique_lock<std::mutex> lk(mtx_);
        nthreads_ -= 1;
        cond_.notify_all();
        cond_.wait(lk, [&] { return nthreads_ == 0; });
      }
    }
  };
}  // namespace pmembench
