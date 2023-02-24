#pragma once

#include <cmath>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pmembench {

  template <typename T, T p> auto constexpr power(T x) -> T {
    if constexpr (p == 0) {
      return 1;
    }
    if constexpr (p == 1) {
      return x;
    }

    T tmp = power<T, p / 2>(x);
    if constexpr ((p % 2) == 0) {
      return tmp * tmp;
    } else {
      return x * tmp * tmp;
    }
  }

  template <typename T, size_t BASE = 1024> inline auto prettyTo(std::string_view pretty) -> T {
    std::string s{pretty};
    std::smatch m;
    if (!std::regex_match(s, m, std::regex{R"((\d+)(K|M|G|T|P)?)"})) {
      std::stringstream ss;
      ss << "Invalid Argument: " << pretty;
      throw std::invalid_argument(ss.str());
    }
    std::istringstream iss(m[1]);
    T result;
    iss >> result;

    auto suffix{m[2].str()};
    if (!suffix.length()) {
      return result;
    }
    if (suffix == "K") {
      return result * BASE;
    }
    if (suffix == "M") {
      return result * power<T, 2>(BASE);
    }
    if (suffix == "G") {
      return result * power<T, 3>(BASE);
    }
    if (suffix == "T") {
      return result * power<T, 4>(BASE);
    }
    if (suffix == "P") {
      return result * power<T, 5>(BASE);
    }

    throw std::runtime_error("must not reach here.");
  }

}  // namespace pmembench
