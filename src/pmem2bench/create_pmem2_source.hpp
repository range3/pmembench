#pragma once
#include <fcntl.h>
#include <libpmem2.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <string_view>

namespace pmb {

inline auto createPmem2SourceFromFsdax(struct pmem2_source** src,
                                       std::string_view path,
                                       off64_t file_size) -> bool {
  int fd;
  fd = open(path.data(), O_RDWR);
  if (fd < 0) {
    fd = open(path.data(), O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 0666);
    if (fd < 0) {
      std::perror(path.data());
      return false;
    }
  }
  if (ftruncate64(fd, file_size) != 0) {
    std::perror("ftruncate64");
    return false;
  }

  if (pmem2_source_from_fd(src, fd) != 0) {
    pmem2_perror("pmem2_source_from_fd");
    return false;
  }
  return true;
}

inline auto createPmem2SourceFromDevdax(struct pmem2_source** src,
                                        std::string_view path) -> bool {
  int fd;
  fd = open(path.data(), O_RDWR);
  if (pmem2_source_from_fd(src, fd) != 0) {
    pmem2_perror("pmem2_source_from_fd");
    return false;
  }
  return true;
}

inline auto createPmem2SourceFromAnonymous(struct pmem2_source** src,
                                           size_t size) -> bool {
  if (pmem2_source_from_anon(src, size) != 0) {
    pmem2_perror("pmem2_source_from_anon");
    return false;
  }
  return true;
}

}  // namespace pmb
