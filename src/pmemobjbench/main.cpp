#include <bits/exception.h>
#include <fmt/core.h>
#include <libpmemobj/base.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <pmembench/barrier.hpp>
#include <pmembench/elapsedtime.hpp>
#include <pmembench/gen_random_string.hpp>
#include <pmembench/pretty_bytes.hpp>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

inline auto memcpyConstructor(PMEMobjpool* pop, void* ptr, void* arg) -> int {
  // auto [src, size] = *static_cast<std::pair<char*, size_t>*>(arg);
  auto* src = static_cast<std::string*>(arg);
  char* dest = static_cast<char*>(ptr);

  pmemobj_memcpy(pop, dest, src->data(), src->size(), PMEMOBJ_F_MEM_NONTEMPORAL);

  return 0;
}

auto main(int argc, char* argv[]) -> int try {
  cxxopts::Options options("pmemobjbench", "Benchmarking pmem using libpmemobj++");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("./pmemobjbench.pool"))
    ("S,file_size", "file size (bytes)", cxxopts::value<std::string>()->default_value("2G"))
    ("t,total", "total access size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("b,block", "block size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("s,stripe", "stripe size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("n,nthreads", "number of threads", cxxopts::value<std::string>()->default_value("1"))
    ("read", "read")
    ("write", "write")
    ("alloc", "only allocation")
    ("r,random", "randomize block access in a strip unit.")
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto file_size = pmembench::prettyTo<uint64_t>(parsed["file_size"].as<std::string>());
  auto block_size = pmembench::prettyTo<uint64_t>(parsed["block"].as<std::string>());
  auto stripe_size = pmembench::prettyTo<uint64_t>(parsed["stripe"].as<std::string>());
  auto nthreads = pmembench::prettyTo<uint64_t>(parsed["nthreads"].as<std::string>());
  auto total_size = pmembench::prettyTo<uint64_t>(parsed["total"].as<std::string>());
  auto nobjects = total_size / block_size;
  bool op_read = parsed.count("read") != 0U;
  bool op_write = parsed.count("write") != 0U;
  bool op_alloc = parsed.count("alloc") != 0U;
  bool op_random = parsed.count("random") != 0U;

  // check arguments
  if (!op_read && !op_write) {
    fmt::print(stderr, "Error: --read or --write required \n");
    exit(1);
  }

  if (((total_size % stripe_size) != 0U) || ((total_size % block_size) != 0U)
      || ((stripe_size % block_size) != 0U) || ((stripe_size % nthreads) != 0U)
      || stripe_size / nthreads < block_size || op_read == op_write) {
    fmt::print(stderr, "Error: Invalid arguments\n", options.help());
    exit(1);
  }

  // clang-format off
  json benchmark_result = {
    {"params", {
      {"path", parsed["path"].as<std::string>()},
      {"fileSize", file_size},
      {"blockSize", block_size},
      {"stripeSize", stripe_size},
      {"nthreads", nthreads},
      {"totalSize", total_size},
      {"accessType", op_read ? "read" : "write"},
      {"onlyAllocation", op_alloc},
      {"accessPattern", op_random ? "random" : "sequential"}
    }},
    {"results", {
      {"success", false},
      {"time", 0.0},
      {"throuput", 0.0},
    }}
  };
  // clang-format on

  struct Object {
    pmem::obj::persistent_ptr<char[]> data;  // NOLINT
  };

  struct Root {
    using obj_vector = pmem::obj::vector<Object>;

    obj_vector objs;
  };

  // create or open a pmemobj pool
  auto const pool_file_path = parsed["path"].as<std::string>();
  constexpr const char* kPoolLayout = "pmemobjbench";
  using pool_type = pmem::obj::pool<Root>;
  pool_type pop;

  if (pool_type::check(pool_file_path, kPoolLayout) == 1) {
    pop = pool_type::open(pool_file_path, kPoolLayout);
  } else {
    pop = pool_type::create(pool_file_path, kPoolLayout, file_size);
  }

  std::string random_string_data = pmembench::generateRandomAlphanumericString(block_size);

  // init root object
  auto r = pop.root();
  pmem::obj::transaction::run(pop, [&] { r->objs.resize(nobjects); });

  // test
  // if (op_write) {
  //   std::string buffer = grs::generateRandomAlphanumericString(block_size);

  //   // // alloc obj test
  //   // pmem::obj::make_persistent_atomic<char[]>(pop, r->objs[0].data,
  //   // block_size);

  //   // // memcpy using non-temporal store
  //   // pmemobj_memcpy(pop.handle(), r->objs[0].data.get(), buffer.data(),
  //   //                buffer.size(), PMEMOBJ_F_MEM_NONTEMPORAL);

  //   std::pair<char*, size_t> source = {buffer.data(), buffer.size()};
  //   pmemobj_xalloc(pop.handle(), r->objs[0].data.raw_ptr(), buffer.size(),
  //                  typeid(char).hash_code(), PMEMOBJ_F_MEM_NONTEMPORAL,
  //                  memcpyConstructor, static_cast<void*>(&source));

  //   fmt::print("write: {}...\n", buffer.substr(0, 16));
  // }

  // if (op_read) {
  //   std::string buffer;
  //   buffer.resize(block_size);

  //   memcpy(buffer.data(), r->objs[0].data.get(), buffer.size());

  //   fmt::print("read: {}...\n", buffer.substr(0, 16));
  // }
  // return 0;

  auto stripe_count = total_size / stripe_size;
  auto strip_unit_size = stripe_size / nthreads;
  auto blocks_in_strip_unit = strip_unit_size / block_size;
  auto blocks_in_stripe = stripe_size / block_size;

  std::vector<size_t> access_offset(blocks_in_strip_unit);
  // 0 1 2 3 4 ...
  std::iota(access_offset.begin(), access_offset.end(), 0);
  // randomize offset
  if (op_random) {
    std::shuffle(access_offset.begin(), access_offset.end(), std::mt19937{std::random_device{}()});
  }

  std::vector<std::thread> workers;
  pmembench::Barrier wait_for_ready(nthreads + 1);
  pmembench::Barrier wait_for_timer(nthreads + 1);
  pmembench::Barrier wait_for_finish(nthreads + 1);
  std::atomic<bool> fail(false);

  int ret;
  for (size_t i = 0; i < nthreads; ++i) {
    workers.emplace_back([&, i] {
      std::string buf = random_string_data;
      wait_for_ready.enter();
      wait_for_timer.enter();

      for (size_t stripe = 0; stripe < stripe_count; ++stripe) {
        size_t strip_ofs = stripe * blocks_in_stripe + i * blocks_in_strip_unit;
        for (size_t j = 0; j < blocks_in_strip_unit; ++j) {
          size_t block_ofs = access_offset[j] + strip_ofs;
          if (op_write) {
            // alloc
            if (op_alloc) {
              pmem::obj::make_persistent_atomic<char[]>(pop, r->objs[block_ofs].data, block_size);
            } else {
              // fmt::format("{:08}", block_ofs).copy(buf.data(), 8);
              // std::pair<char*, size_t> source = {buf.data(), buf.size()};
              ret = pmemobj_xalloc(pop.handle(), r->objs[block_ofs].data.raw_ptr(), buf.size(), 0,
                                   0, memcpyConstructor, static_cast<void*>(&buf));
              if (ret != 0) {
                perror("pmemobj_xalloc");
                fail.store(true, std::memory_order_release);
                break;
              }
            }

            // if (!op_alloc) {
            //   // memcpy using non-temporal store
            //   pmemobj_memcpy(pop.handle(), r->objs[block_ofs].data.get(),
            //                  buf.data(), block_size,
            //                  PMEMOBJ_F_MEM_NONTEMPORAL);
            // }

          } else {
            memcpy(static_cast<void*>(buf.data()),
                   static_cast<const void*>(r->objs[block_ofs].data.get()), block_size);
            // fmt::print("{} = {}\n", block_ofs, buf.substr(0, 8));
          }
        }
        if (fail.load(std::memory_order_acquire)) {
          break;
        }
      }
      wait_for_finish.enter();
    });
  }

  wait_for_ready.enter();
  pmembench::ElapsedTime elapsed_time;
  elapsed_time.reset();
  wait_for_timer.enter();
  wait_for_finish.enter();
  elapsed_time.freeze();

  for (auto& worker : workers) {
    worker.join();
  }

  benchmark_result["results"]["success"] = !fail;
  if (!fail) {
    benchmark_result["results"]["time"] = elapsed_time.msec();
    benchmark_result["results"]["throuput"] = static_cast<double>(total_size) / elapsed_time.sec();
  }
  if (parsed.count("prettify") != 0U) {
    std::cout << std::setw(2);
  }
  std::cout << benchmark_result << std::endl;

  return 0;
} catch (const std::exception& e) {
  fmt::print(stderr, "Exception {}\n", e.what());
  return -1;
}
