include(CMakePackageConfigHelpers)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/lazperf-config-version.cmake
    VERSION ${LAZPERF_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_file(${LAZPERF_CMAKE_DIR}/lazperf-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/lazperf-config.cmake COPYONLY
)

function(lazperf_install_library _target)
    install(
        TARGETS
            ${_target}
        EXPORT
            lazperf-targets
        LIBRARY DESTINATION lib
    )
    target_include_directories(${_target} INTERFACE $<INSTALL_INTERFACE:include>)
endfunction()

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
