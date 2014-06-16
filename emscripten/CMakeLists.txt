include_directories(../tlaz)

if (NOT EMSCRIPTEN)
	message(FATAL_ERROR "You should not be including this directory for non emscripten builds")
endif()

set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

# we need the bind flag to generate bindings
set(CMAKE_CXX_FLAGS "--bind -s DISABLE_EXCEPTION_CATCHING=2 ${CMAKE_CXX_FLAGS}")

add_executable(laz-perf laz-perf.cpp)
target_link_libraries(laz-perf ${ALL_LIBRARIES})

SET_TARGET_PROPERTIES(laz-perf PROPERTIES SUFFIX .js)
