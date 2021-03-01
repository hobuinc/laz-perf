if(WIN32)
cmake_minimum_required(VERSION 2.8.7)
else()
cmake_minimum_required(VERSION 2.6.0)
endif()

project(LAZPERF)

#------------------------------------------------------------------------------
# internal cmake settings
#------------------------------------------------------------------------------

set(CMAKE_COLOR_MAKEFILE ON)

# Allow advanced users to generate Makefiles printing detailed commands
mark_as_advanced(CMAKE_VERBOSE_MAKEFILE)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})


#------------------------------------------------------------------------------
# general settings
#------------------------------------------------------------------------------

set(LAZPERF_VERSION_MAJOR "1")
set(LAZPERF_VERSION_MINOR "5")
set(LAZPERF_VERSION_PATCH "0")
set(LAZPERF_VERSION "${LAZPERF_VERSION_MAJOR}.${LAZPERF_VERSION_MINOR}.${LAZPERF_VERSION_PATCH}")
set(CMAKE_PROJECT_VERSION "${CMAKE_PROJECT_VERSION}")

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

add_subdirectory(cpp)
