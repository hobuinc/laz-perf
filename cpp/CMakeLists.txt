
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#------------------------------------------------------------------------------
# test harness settings
#------------------------------------------------------------------------------

# Name of test suite runner
set(LAZPERF_UNIT_TEST lazperf_tests)

#------------------------------------------------------------------------------
# Platform and compiler specific settings
#------------------------------------------------------------------------------

# if we have an emscripten build, make sure the emscripten tag is set

# Emscripten comes with its own POSIX headers so to trigger their
# own includes, treat it like a posix system
#
#if (EMSCRIPTEN)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEMSCRIPTEN_BUILD -DEMSCRIPTEN_LINUX")
#endif()

set(LAZPERF_SHARED_LIB lazperf)
set(LAZPERF_STATIC_LIB lazperf_s)

add_subdirectory(laz-perf)
add_subdirectory(examples)
add_subdirectory(benchmarks)
if (EMSCRIPTEN)
    add_subdirectory(emscripten)
endif()

if (WITH_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

#
# Install
#
include(CMakePackageConfigHelpers)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/lazperf-config-version.cmake
    VERSION ${LAZPERF_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(../cmake/lazperf-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/lazperf-config.cmake
    COPYONLY
)

install(
    FILES
        laz-perf/lazperf.hpp
        laz-perf/filestream.hpp
        laz-perf/vlr.hpp
    DESTINATION
        include/lazperf
)

install(
    TARGETS
        ${LAZPERF_SHARED_LIB}
    EXPORT
        lazperf-targets
    LIBRARY DESTINATION lib
)

install(
    EXPORT
        lazperf-targets
    FILE
        lazperf-targets.cmake
    NAMESPACE
        LAZPERF::
    DESTINATION
        lib/cmake/LAZPERF
)

target_include_directories(${LAZPERF_SHARED_LIB}
    INTERFACE
        $<INSTALL_INTERFACE:include>)

#
# cmake file handling
#

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/lazperf-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/lazperf-config-version.cmake
    DESTINATION
        lib/cmake/LAZPERF
)

