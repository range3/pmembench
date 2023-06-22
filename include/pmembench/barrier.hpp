#pragma once

#include <condition_variable>
#include <mutex>

namespace pmembench {
  class Barrier {
    const size_t nthreads_;
    size_t waits_;
    bool sense_;
    std::mutex mtx_;
    std::condition_variable cond_;

  public:
    Barrier(size_t nthreads) : nthreads_(nthreads), waits_(nthreads), sense_(true) {}

    void wait() {
      std::unique_lock<std::mutex> lk(mtx_);
      bool current_sense = sense_;

      if (--waits_ == 0) {
        waits_ = nthreads_;
        sense_ = !sense_;
        cond_.notify_all();
      } else {
        cond_.wait(lk, [&] { return current_sense != sense_; });
      }
    }
  };
}  // namespace pmembench
