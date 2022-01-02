include_directories(../lazperf)

if (NOT EMSCRIPTEN)
	message(FATAL_ERROR "You should not be including this directory for non emscripten builds")
endif()

# we need the bind flag to generate bindings
set(CMAKE_CXX_FLAGS "--bind -s TOTAL_MEMORY=167772160 -s ${CMAKE_CXX_FLAGS}")

# If wasm build was requested, then generate native output
if (WASM)
  message(STATUS "WASM Emscripten output has been enabled")
  set(CMAKE_CXX_FLAGS "-s WASM=1 -s \"BINARYEN_METHOD='native-wasm'\" ${CMAKE_CXX_FLAGS}")
else(WASM)
  message(STATUS "JS Emscripten output has been enabled")
  set(CMAKE_CXX_FLAGS "-s WASM=0 ${CMAKE_CXX_FLAGS}")
endif()

add_executable(laz-perf laz-perf.cpp)
target_link_libraries(laz-perf ${LAZPERF_STATIC_LIB})
if (WIN32)
  target_compile_definitions(laz-perf PRIVATE LAZPERF_LIB_STATIC)
endif()

if (WASM)
  SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .html)
else()
  SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .asm.js)
endif()
