find_package(doctest REQUIRED)
find_package(Threads REQUIRED)

macro(register_test testname testfilepath)
  add_executable(${testname} ${CMAKE_CURRENT_LIST_DIR}/${testfilepath})
  target_link_libraries(${testname} PRIVATE
    Threads::Threads
    cxxopts::cxxopts
    fmt::fmt
    nlohmann_json::nlohmann_json
    doctest::doctest
    pmembench::pmembench
  )
  target_include_directories(${testname} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  )
  add_test(
    NAME ${testname}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${testname})
endmacro()

file(GLOB_RECURSE tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*_test.cpp")
foreach(test ${tests})
    message(STATUS "Test executable is found: ${test}")
    get_filename_component(testname ${test} NAME_WE)
    register_test(${testname} ${test})
endforeach(test)
