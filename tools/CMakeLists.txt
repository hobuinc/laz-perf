
add_executable(random random.cpp)

target_include_directories(random PRIVATE ../cpp/laz-perf)
lazperf_target_compile_settings(random)
target_link_libraries(random PRIVATE lazperf)

