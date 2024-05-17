#pragma once
#include <atomic>
#include <vector>

class spin_barrier {
  spin_barrier() = delete;
  spin_barrier(const spin_barrier&) = delete;
  spin_barrier& operator=(const spin_barrier&) = delete;

 public:
  static int tid2idx(size_t i) { return i * 64 / sizeof(int); }

 public:
  explicit spin_barrier(size_t threads)
      : threads_(static_cast<int>(threads)),
        waits_(static_cast<int>(threads)),
        sense_(false),
        my_sense_(tid2idx(threads)) {
    for (size_t i = 0; i < threads; ++i) {
      my_sense_[tid2idx(i)] = true;
    }
  }

  void wait(size_t i) {
    int sense = my_sense_[tid2idx(i)];
    if (waits_.fetch_sub(1) == 1) {
      waits_.store(threads_);
      sense_.store(sense != 0, std::memory_order_release);
    } else {
      while (sense != sense_)
        ;
    }
    my_sense_[tid2idx(i)] = !sense;
  }

 private:
  const int threads_;
  std::atomic<int> waits_;
  std::atomic<bool> sense_;
  std::vector<int> my_sense_;
};
