
file(GLOB SRCS
    lazperf/*.cpp
    lazperf/detail/*.cpp
)

if (BUILD_SHARED_LIBS)
    lazperf_add_library(lazperf SHARED ${SRCS})
else()
    lazperf_add_library(lazperf STATIC ${SRCS})
endif()
add_library(lazperf::lazperf ALIAS lazperf)

install(
    FILES
        lazperf/lazperf.hpp
        lazperf/filestream.hpp
        lazperf/header.hpp
        lazperf/readers.hpp
        lazperf/vlr.hpp
        lazperf/writers.hpp
    DESTINATION
        include/lazperf
)
install(
    FILES
        lazperf/lazperf_user_base.hpp
    DESTINATION
        include/lazperf
    RENAME
        lazperf_base.hpp
)

add_subdirectory(benchmarks)
add_subdirectory(tools)
if (EMSCRIPTEN)
    add_subdirectory(emscripten)
endif()

if (WITH_TESTS)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(examples)
endif()

