#include <fcntl.h>
#include <fmt/core.h>
#include <libpmem2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <pmembench/barrier.hpp>
#include <pmembench/elapsedtime.hpp>
#include <pmembench/gen_random_string.hpp>
#include <pmembench/pretty_bytes.hpp>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "create_pmem2_source.hpp"

using json = nlohmann::json;

const std::vector<std::string> kSourceType = {"fsdax", "devdax", "anon"};

extern "C" {
void memcpy_erms(void* dst, void* src, size_t size);
}

thread_local char buf[2 * 1024 * 1024];
// thread_local char* buf;

auto main(int argc, char* argv[]) -> int {
  cxxopts::Options options("pmem2bench",
                           "Benchmarking persistent memory resident array of "
                           "blocks using libpmem2");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("dry-run", "dry run")
    ("source", "select (fsdax | devdax | anon)", cxxopts::value<std::string>()->default_value("fsdax"))
    ("p,path", "/path/to/file or device", cxxopts::value<std::string>()->default_value("./pmem2bench.file"))
    ("S,file_size", "file size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("b,block", "block size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("s,stripe", "stripe size (bytes)", cxxopts::value<std::string>()->default_value("512"))
    ("n,nthreads", "number of threads", cxxopts::value<std::string>()->default_value("1"))
    ("t,total", "total read/write size (bytes)", cxxopts::value<std::string>()->default_value("1G"))
    ("read", "read")
    ("write", "write")
    ("r,random", "randomize block access in a strip unit.")
    ("v,verbose", "verbose output")
    ("prettify", "prettify the json output")
    ("g,granularity", "granularity realted to power-fail protected domain should be (page | cacheline | byte)", cxxopts::value<std::string>()->default_value("page"))
    ("non-temporal", "use non-temporal stores")
    // ("a,align", "Alignment", cxxopts::value<size_t>()->default_value("4096"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);
  if (result.count("help") != 0U) {
    fmt::print("{}\n", options.help());
    fmt::print("You can use byte suffixes: K|M|G|T|P\n");
    return 0;
  }

  // convert pretty bytes to uint64_t
  auto file_size = pmembench::prettyTo<uint64_t>(result["file_size"].as<std::string>());
  auto block_size = pmembench::prettyTo<uint64_t>(result["block"].as<std::string>());
  auto stripe_size = pmembench::prettyTo<uint64_t>(result["stripe"].as<std::string>());
  auto nthreads = pmembench::prettyTo<uint64_t>(result["nthreads"].as<std::string>());
  auto total_size = pmembench::prettyTo<uint64_t>(result["total"].as<std::string>());
  // auto op_alignment = result["align"].as<size_t>();
  bool op_read = result.count("read") != 0U;
  bool op_write = result.count("write") != 0U;
  bool op_random = result.count("random") != 0U;
  bool op_verbose = result.count("verbose") != 0U;
  bool op_dry_run = result.count("dry-run") != 0U;
  enum pmem2_granularity op_granularity;
  if (result["granularity"].as<std::string>() == "page") {
    op_granularity = PMEM2_GRANULARITY_PAGE;
  } else if (result["granularity"].as<std::string>() == "cacheline") {
    op_granularity = PMEM2_GRANULARITY_CACHE_LINE;
  } else if (result["granularity"].as<std::string>() == "byte") {
    op_granularity = PMEM2_GRANULARITY_BYTE;
  } else {
    fmt::print(stderr, "Error: --granularity should be (page | cacheline | byte)\n");
    exit(1);
  }
  bool op_non_temporal = result.count("non-temporal") != 0U;

  // check arguments
  if (!op_read && !op_write) {
    fmt::print(stderr, "Error: --read or --write required \n");
    exit(1);
  }
  auto op_source = result["source"].as<std::string>();
  if (std::find(kSourceType.begin(), kSourceType.end(), op_source) == kSourceType.end()) {
    fmt::print(stderr, "Error: unknown --source type {}\n", op_source);
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
      {"source", result["source"].as<std::string>()},
      {"path", result["path"].as<std::string>()},
      {"fileSize", file_size},
      {"blockSize", block_size},
      {"stripeSize", stripe_size},
      {"nthreads", nthreads},
      {"totalSize", total_size},
      {"accessType", op_read ? "read" : "write"},
      {"accessPattern", op_random ? "random" : "sequential"},
      {"granularity", result["granularity"].as<std::string>()},
      {"nonTemporal", op_non_temporal},
    }},
    {"results", {
      {"success", false},
      {"time", 0.0},
      {"throuput", 0.0},
      {"addr", 0ULL},
    }}
  };
  // clang-format on

  // print benchmark info
  if (op_verbose) {
    fmt::print("block size: {}\n", block_size);
    fmt::print("stripe size: {}\n", stripe_size);
    fmt::print("nthreads: {}\n", nthreads);
    fmt::print("total_size: {}\n", total_size);
    fmt::print("access pattern: {}\n", op_random ? "random" : "sequential");
  }

  // create the pmem2 pool or open it if it already exists
  struct pmem2_source* src;
  if (!op_dry_run) {
    if (op_source == "fsdax") {
      if (!pmb::createPmem2SourceFromFsdax(&src, result["path"].as<std::string>(),
                                           static_cast<off64_t>(file_size))) {
        exit(1);
      }
    } else if (op_source == "devdax") {
      if (!pmb::createPmem2SourceFromDevdax(&src, result["path"].as<std::string>())) {
        exit(1);
      }
    } else {
      if (!pmb::createPmem2SourceFromAnonymous(&src, file_size)) {
        exit(1);
      }
    }
  }

  // pmem2 config
  struct pmem2_config* cfg;
  struct pmem2_map* map;
  char* addr = nullptr;
  pmem2_memcpy_fn memcpy_fn;
  if (!op_dry_run) {
    if (pmem2_config_new(&cfg) != 0) {
      pmem2_perror("pmem2_config_new");
      exit(1);
    }

    if (pmem2_config_set_required_store_granularity(cfg, op_granularity) != 0) {
      pmem2_perror("pmem2_config_set_required_store_granularity");
      exit(1);
    }

    if (pmem2_map_new(&map, cfg, src) != 0) {
      pmem2_perror("pmem2_map_new");
      exit(1);
    }

    // pmem2 functions
    addr = static_cast<char*>(pmem2_map_get_address(map));
    memcpy_fn = pmem2_get_memcpy_fn(map);
  }
  unsigned int memcpy_flag = op_non_temporal ? PMEM2_F_MEM_NONTEMPORAL : PMEM2_F_MEM_TEMPORAL;

  // how many elements fit into the file?
  // size_t map_size = pmem2_map_get_size(map);
  // fmt::print("map size: {}\n", map_size);

  auto random_string_data = pmembench::generateRandomAlphanumericString(block_size);

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

  for (size_t i = 0; i < nthreads; ++i) {
    workers.emplace_back([&, i] {
      // buf = (char*)malloc(random_string_data.size());
      // buf = (char*)malloc(2*1024*1024);
      // buf = (char*)aligned_alloc(op_alignment, 2*1024*1024);
      // buf = gbuf + i*2*1024*1024;
      // uintptr_t iptr = reinterpret_cast<uintptr_t>(buf);
      // fmt::print("{}: {}, {}, {}, {}\n", i, iptr, iptr%4*1024,
      // iptr%2*1024*1024, iptr/(2*1024*1024)); std::string buf =
      // random_string_data; memset(buf, 'a', 1*1024*1024);
      memcpy(buf, random_string_data.data(), random_string_data.size());
      wait_for_ready.enter();
      wait_for_timer.enter();

      for (size_t stripe = 0; stripe < stripe_count; ++stripe) {
        size_t strip_ofs = stripe * blocks_in_stripe + i * blocks_in_strip_unit;
        for (size_t j = 0; j < blocks_in_strip_unit; ++j) {
          size_t block_ofs = access_offset[j] + strip_ofs;
          if (op_dry_run) {
            if (op_verbose) {
              fmt::print("t:{}\tblock_idx:{}\tblock_size:{}\n", i, block_ofs, block_size);
            }

          } else if (op_write) {
            memcpy_fn(addr + block_ofs * block_size, buf, block_size, memcpy_flag);
            // memcpy_fn(addr + block_ofs * block_size, buf.data(), block_size,
            //           memcpy_flag);
          } else {
            memcpy(buf, addr + block_ofs * block_size, block_size);
            // memcpy_erms(&buf[0], addr + block_ofs * block_size, block_size);
            // std::copy(addr + block_ofs * block_size, addr + (block_ofs+1) *
            // block_size, buf);
          }
        }
      }
      wait_for_finish.enter();
      // free(buf);
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
    benchmark_result["results"]["addr"] = reinterpret_cast<uintptr_t>(addr);

    if (op_verbose) {
      fmt::print("Elapsed Time: {} msec\n", elapsed_time.msec());
      fmt::print("Throuput: {} bytes/sec\n", static_cast<double>(total_size) / elapsed_time.sec());
    }
  }
  if (result.count("prettify") != 0U) {
    std::cout << std::setw(2);
  }
  std::cout << benchmark_result << std::endl;

  if (!op_dry_run) {
    pmem2_map_delete(&map);
    pmem2_config_delete(&cfg);
    int fd;
    if (pmem2_source_get_fd(src, &fd) == 0) {
      close(fd);
    }
    pmem2_source_delete(&src);
  }
  return 0;
}
