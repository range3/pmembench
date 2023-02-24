set(EXE_TARGET_NAME pmem2bench)

set(${EXE_TARGET_NAME}_SRC # cmf: sortable
                           main.cpp
                           memcpy_64.S
                           )

add_executable(${EXE_TARGET_NAME})

target_sources(${EXE_TARGET_NAME} PRIVATE ${${EXE_TARGET_NAME}_SRC})

target_compile_features(${EXE_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${EXE_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(cxxopts REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Libpmem2 REQUIRED IMPORTED_TARGET libpmem2)

target_link_libraries(${EXE_TARGET_NAME} PRIVATE
  Threads::Threads
  cxxopts::cxxopts
  fmt::fmt
  nlohmann_json::nlohmann_json
  pmembench::pmembench
  PkgConfig::Libpmem2)

install(TARGETS ${EXE_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
