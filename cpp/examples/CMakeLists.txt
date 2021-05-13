add_executable(point10 point10.cpp)
target_include_directories(point10
    PRIVATE
        ../lazperf
)
target_link_libraries(point10
    PRIVATE
        ${LAZPERF_STATIC_LIB}
)
lazperf_target_compile_settings(point10)

add_executable(readlaz readlaz.cpp)
target_include_directories(readlaz
    PRIVATE
        ../lazperf
)
target_link_libraries(readlaz
    PRIVATE
        ${LAZPERF_STATIC_LIB}
)
lazperf_target_compile_settings(readlaz)

if (EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "-s ${CMAKE_CXX_FLAGS}")
	SET_TARGET_PROPERTIES(point10 PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(readlaz PROPERTIES SUFFIX .js)
endif()
