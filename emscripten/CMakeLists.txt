include_directories(../laz-perf)

if (NOT EMSCRIPTEN)
	message(FATAL_ERROR "You should not be including this directory for non emscripten builds")
endif()

set(ALL_LIBRARIES ${LAZPERF_DEPENDENT_LIBRARIES})

# we need the bind flag to generate bindings
set(CMAKE_CXX_FLAGS "--bind -s TOTAL_MEMORY=104857600 -s DISABLE_EXCEPTION_CATCHING=2 ${CMAKE_CXX_FLAGS}")

add_executable(laz-perf laz-perf.cpp)
target_link_libraries(laz-perf ${ALL_LIBRARIES})

SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .js)
