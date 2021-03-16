include_directories(../laz-perf)

set(ALL_LIBRARIES ${LAZPERF_DEPENDENT_LIBRARIES} lazperf)

add_executable(simple simple.cpp)
target_link_libraries(simple ${ALL_LIBRARIES})

add_executable(point10 point10.cpp)
target_link_libraries(point10 ${ALL_LIBRARIES})

add_executable(readlaz readlaz.cpp)
target_link_libraries(readlaz ${ALL_LIBRARIES})

add_executable(dynamic dynamic.cpp)
target_link_libraries(dynamic ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "-s DISABLE_EXCEPTION_CATCHING=2 ${CMAKE_CXX_FLAGS}")
	SET_TARGET_PROPERTIES(simple PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(point10 PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(readlaz PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(dynamic PROPERTIES SUFFIX .js)
endif()
