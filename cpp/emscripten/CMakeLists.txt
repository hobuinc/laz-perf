include_directories(../lazperf)

if (NOT EMSCRIPTEN)
	message(FATAL_ERROR "You should not be including this directory for non emscripten builds")
endif()

# We need the bind flag to generate bindings.  These particular numbers aren't
# terribly important as they can be overridden by the user at runtime.
set(CMAKE_CXX_FLAGS "--bind")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ASSERTIONS=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s INITIAL_MEMORY=262144")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s TOTAL_STACK=65536")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1")

# If wasm build was requested, then generate native output
if (WASM)
  message(STATUS "WASM Emscripten output has been enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s \"BINARYEN_METHOD='native-wasm'\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MODULARIZE=1")
  if (ESM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_ES6=1")
  else (ESM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_NAME=createLazPerf")
  endif()
else (WASM)
  message(STATUS "JS Emscripten output has been enabled")
  set(CMAKE_CXX_FLAGS "-s WASM=0 ${CMAKE_CXX_FLAGS}")
endif()

message("FLAGS: ${CMAKE_CXX_FLAGS}")

add_executable(laz-perf laz-perf.cpp)
target_link_libraries(laz-perf ${LAZPERF_STATIC_LIB})

if (WASM)
  SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .js)
else()
  SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .asm.js)
endif()
