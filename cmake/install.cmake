include(CMakePackageConfigHelpers)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/lazperf-config-version.cmake
    VERSION ${LAZPERF_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(${PROJECT_SOURCE_DIR}/cmake/lazperf-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/lazperf-config.cmake
    COPYONLY
)

install(
    FILES
        ${PROJECT_SOURCE_DIR}/cpp/lazperf/lazperf.hpp
        ${PROJECT_SOURCE_DIR}/cpp/lazperf/filestream.hpp
        ${PROJECT_SOURCE_DIR}/cpp/lazperf/vlr.hpp
        ${PROJECT_SOURCE_DIR}/cpp/lazperf/io.hpp
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
