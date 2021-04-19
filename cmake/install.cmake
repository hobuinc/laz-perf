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
        ${LAZPERF_LIB}
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

target_include_directories(${LAZPERF_LIB}
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
