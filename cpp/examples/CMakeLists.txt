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
if (WIN32)
    target_compile_definitions(point10 PRIVATE LAZPERF_LIB_STATIC)
endif()

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
if (WIN32)
    target_compile_definitions(readlaz PRIVATE LAZPERF_LIB_STATIC)
endif()

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(point10 PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(readlaz PROPERTIES SUFFIX .js)
endif()
