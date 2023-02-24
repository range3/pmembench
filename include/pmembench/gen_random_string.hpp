#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <random>
#include <string>

namespace pmembench {

  template <typename T = std::mt19937> inline auto randomGenerator() -> T {
    auto constexpr kSeedBytes = sizeof(typename T::result_type) * T::state_size;
    auto constexpr kSeedLen = kSeedBytes / sizeof(std::seed_seq::result_type);
    auto seed = std::array<std::seed_seq::result_type, kSeedLen>();
    auto dev = std::random_device();
    std::generate_n(begin(seed), kSeedLen, std::ref(dev));
    auto seed_seq = std::seed_seq(begin(seed), end(seed));
    return T{seed_seq};
  }

  inline auto generateRandomAlphanumericString(std::size_t len) -> std::string {
    static constexpr auto kChars
        = "0123456789"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz";
    thread_local auto rng = randomGenerator<>();
    auto dist = std::uniform_int_distribution{{}, std::strlen(kChars) - 1};
    auto result = std::string(len, '\0');
    std::generate_n(begin(result), len, [&]() { return kChars[dist(rng)]; });
    return result;
  }

}  // namespace pmembench
