
macro(LAZPERF_ADD_TEST _name)
  add_executable(${_name} ${_name}.cpp Charbuf.cpp)
  target_link_libraries(${_name} gtest lazperf)
  target_include_directories(${_name} PRIVATE gtest/gtest-1.10.0/include)
  target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  lazperf_target_compile_settings(${_name})
  add_test(NAME ${_name} COMMAND ${PROJECT_BINARY_DIR}/cpp/test/${_name}
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../..")
endmacro(LAZPERF_ADD_TEST)

configure_file(test_main.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/test_main.hpp)
add_subdirectory(gtest/gtest-1.10.0)

LAZPERF_ADD_TEST(io_tests)
LAZPERF_ADD_TEST(lazperf_tests)
LAZPERF_ADD_TEST(stream_tests)

