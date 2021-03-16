
#------------------------------------------------------------------------------
# test harness settings
#------------------------------------------------------------------------------

# Name of test suite runner
set(LAZPERF_UNIT_TEST lazperf_tests)

#------------------------------------------------------------------------------
# Platform and compiler specific settings
#------------------------------------------------------------------------------

if(WIN32)
  if (MSVC)

      add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
      add_definitions(-D_CRT_NONSTDC_NO_WARNING)
      add_definitions(-D_SCL_SECURE_NO_WARNINGS)

    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      string(REGEX REPLACE "/W[0-4]" "/W4"
        CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
  endif()

  set(LAZPERF_PLATFORM_WIN32 1)

else()
  # not WIN32

  # Recommended C++ compilation flags
  # -Weffc++
  set(LAZPERF_COMMON_CXX_FLAGS
      "-std=c++11 -Wextra -Wall -Wpointer-arith -Wcast-align -Wcast-qual -Wfloat-equal -Wredundant-decls -Wno-long-long -fPIC -pedantic")

    if (CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LAZPERF_COMMON_CXX_FLAGS}")
        set(LAZPERF_COMPILER_GCC 1)
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LAZPERF_COMMON_CXX_FLAGS}")
        set(LAZPERF_COMPILER_CLANG 1)
    endif()

#
# C++11 checking
#

endif(WIN32)

if (APPLE)
    set(SO_EXT dylib)
    set(CMAKE_FIND_FRAMEWORK "LAST")
elseif(WIN32)
    set(SO_EXT dll)
else()
    set(SO_EXT so)
endif(APPLE)

set(LAZPERF_DEPENDENT_LIBRARIES "")

# if we have an emscripten build, make sure the emscripten tag is set

# Emscripten comes with its own POSIX headers so to trigger their
# own includes, treat it like a posix system
#
if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEMSCRIPTEN_BUILD -DEMSCRIPTEN_LINUX")
endif()

add_subdirectory(laz-perf)
add_subdirectory(examples)
add_subdirectory(benchmarks)
if (EMSCRIPTEN)
    add_subdirectory(emscripten)
elseif (WITH_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if (WIN32 AND NOT CYGWIN)
    set (DEF_INSTALL_CMAKE_DIR cmake)
else ()
    set (DEF_INSTALL_CMAKE_DIR lib/cmake/lazperf)
endif ()

set (INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
set (INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

configure_file (lazperf-config.cmake.in "lazperf-config.cmake" @ONLY)
configure_file (lazperf-configVersion.cmake.in "lazperf-configVersion.cmake" @ONLY)

# Install cmake files.
install(
    FILES
        ${CMAKE_BINARY_DIR}/cpp/lazperf-config.cmake
        ${CMAKE_BINARY_DIR}/cpp/lazperf-configVersion.cmake
    DESTINATION
        "${INSTALL_CMAKE_DIR}"
    COMPONENT
        dev
)

# Install source headers.
install(
    DIRECTORY
        laz-perf
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include
)

