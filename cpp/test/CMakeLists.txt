set(GTEST_DIR gtest/gtest-10-06-21)

macro(LAZPERF_ADD_TEST _name)
  add_executable(${_name} ${_name}.cpp)
  target_link_libraries(${_name} gtest ${LAZPERF_STATIC_LIB})
  target_include_directories(${_name} PRIVATE ${GTEST_DIR}/include)
  target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  lazperf_target_compile_settings(${_name})
  add_test(NAME ${_name} COMMAND ${PROJECT_BINARY_DIR}/cpp/test/${_name}
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../..")
endmacro(LAZPERF_ADD_TEST)

configure_file(test_main.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/test_main.hpp)
add_subdirectory(${GTEST_DIR})

LAZPERF_ADD_TEST(io_tests)
LAZPERF_ADD_TEST(lazperf_tests)
LAZPERF_ADD_TEST(stream_tests)

