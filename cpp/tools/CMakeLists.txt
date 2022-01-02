
add_executable(random random.cpp)

target_include_directories(random PRIVATE ../lazperf)
lazperf_target_compile_settings(random)
target_link_libraries(random PRIVATE ${LAZPERF_STATIC_LIB})
if (WIN32)
  target_compile_definitions(random PRIVATE LAZPERF_LIB_STATIC)
endif()
