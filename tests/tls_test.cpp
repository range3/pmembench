
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "pmembench/tls.hpp"

#include <doctest/doctest.h>

#include <thread>
#include <vector>

TEST_CASE("TSP") {
  pmembench::thread_specific_ptr<int> tsp;

  CHECK(tsp.get() == nullptr);
  tsp.reset(new int(10));
  CHECK(tsp.get() != nullptr);
  CHECK(*tsp == 10);
}

TEST_CASE("TSP multithread") {
  constexpr size_t nthreads = 10;
  std::vector<std::thread> threads;

  pmembench::thread_specific_ptr<int> tsp;

  for (size_t i = 0; i < nthreads; ++i) {
    threads.emplace_back([&, i] {
      tsp.reset(new int(i));
      CHECK_EQ(*tsp, i);
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}

TEST_CASE("TLV multithread") {
  constexpr size_t nthreads = 10;
  std::vector<std::thread> threads;

  pmembench::thread_local_value<int> tlv(100);

  for (size_t i = 0; i < nthreads; ++i) {
    threads.emplace_back([&, i] {
      CHECK_EQ(tlv.get(), 100);
      tlv.set(i);
      CHECK_EQ(tlv.get(), i);
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}
