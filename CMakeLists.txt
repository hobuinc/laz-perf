cmake_minimum_required(VERSION 3.5)

project(LAZPERF VERSION 2.0)

# Allow advanced users to generate Makefiles printing detailed commands
mark_as_advanced(CMAKE_VERBOSE_MAKEFILE)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#------------------------------------------------------------------------------
# general settings
#------------------------------------------------------------------------------

set(LAZPERF_LIB_SOVERSION ${LAZPERF_VERSION_MAJOR})
set(LAZPERF_C_LIB_SOVERSION ${LAZPERF_VERSION_MAJOR})
# Choose package components
set(WITH_TESTS TRUE CACHE BOOL "Choose if LAZPERF unit tests should be built")

if(WITH_TESTS)
    if(NOT EMSCRIPTEN)
        enable_testing()
        file (DOWNLOAD
            "https://github.com/PDAL/data/raw/master/autzen/autzen.laz"
            "cpp/test/raw-sets/autzen.laz")
    endif()
endif()

include(${PROJECT_SOURCE_DIR}/cmake/compiler_options.cmake)

add_subdirectory(cpp)
