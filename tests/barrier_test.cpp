#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "pmembench/barrier.hpp"

#include <doctest/doctest.h>

#include <thread>
#include <vector>

#include "pmembench/sense_barrier.hpp"

TEST_CASE("cond wait barrier") {
  using namespace pmembench;

  constexpr size_t nthreads = 10;
  std::vector<std::thread> threads;
  Barrier barrier(nthreads);

  for (size_t i = 0; i < nthreads; ++i) {
    threads.emplace_back([&, i] {
      for (size_t j = 0; j < 1000; ++j) {
        barrier.wait();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}

TEST_CASE("SenseBarrier") {
  using namespace pmembench;

  constexpr size_t nthreads = 10;
  std::vector<std::thread> threads;
  SenseBarrier barrier(nthreads);

  for (size_t i = 0; i < nthreads; ++i) {
    threads.emplace_back([&, i] {
      for (size_t j = 0; j < 1000; ++j) {
        barrier.wait();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}

int thread_local_var_init_test() {
  thread_local int sense = 1;
  sense ^= 1;
  return sense;
}

TEST_CASE("thread_local variable initialization") {
  std::thread t1([]() {
    for (size_t i = 0; i < 1000; ++i) {
      CHECK(thread_local_var_init_test() == (i & 1));
    }
  });

  std::thread t2([]() {
    for (size_t i = 0; i < 1000; ++i) {
      CHECK(thread_local_var_init_test() == (i & 1));
    }
  });

  t1.join();
  t2.join();
}

class HasThreadLocalVariable {
public:
  int call() {
    thread_local int var = 0;
    return ++var;
  }

  int static_call() {
    static int var = 0;
    return ++var;
  }
};

TEST_CASE("static var in class instance") {
  HasThreadLocalVariable c1, c2;

  CHECK_EQ(c1.static_call(), 1);
  CHECK_EQ(c1.static_call(), 2);
  CHECK_EQ(c1.static_call(), 3);

  CHECK_EQ(c2.static_call(), 4);
  CHECK_EQ(c2.static_call(), 5);
}

TEST_CASE("thread_local_var in class instance") {
  HasThreadLocalVariable c1, c2;

  CHECK_EQ(c1.call(), 1);
  CHECK_EQ(c1.call(), 2);
  CHECK_EQ(c1.call(), 3);

  CHECK_EQ(c2.call(), 4);
  CHECK_EQ(c2.call(), 5);
}
