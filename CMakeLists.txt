#
# top-level CMake configuration file for LAZPERF
#
# (based originally on the libLAS files copyright Mateusz Loskot)

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

set(LAZPERF_VERSION_MAJOR "0")
set(LAZPERF_VERSION_MINOR "0")
set(LAZPERF_VERSION_PATCH "1")

set(LAZPERF_VERSION "${LAZPERF_VERSION_MAJOR}.${LAZPERF_VERSION_MINOR}.${LAZPERF_VERSION_PATCH}")
set(LAZPERF_LIB_SOVERSION ${LAZPERF_VERSION_MAJOR})
set(LAZPERF_C_LIB_SOVERSION ${LAZPERF_VERSION_MAJOR})

set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)


#------------------------------------------------------------------------------
# misc user options
#------------------------------------------------------------------------------

# Choose package components
set(WITH_TESTS TRUE CACHE BOOL "Choose if LAZPERF unit tests should be built")


#------------------------------------------------------------------------------
# test harness settings
#------------------------------------------------------------------------------

# Name of test suite runner
set(LAZPERF_UNIT_TEST lazperf_tests)




#------------------------------------------------------------------------------
# General build settings
#------------------------------------------------------------------------------

# note we default to debug mode
if(NOT MSVC_IDE)
  if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
  endif()
  message(STATUS "Setting LAZPERF build type - ${CMAKE_BUILD_TYPE}")
endif()

set(PROJCL_BUILD_TYPE ${CMAKE_BUILD_TYPE})


#------------------------------------------------------------------------------
# Platform and compiler specific settings
#------------------------------------------------------------------------------

if(WIN32)
  if (MSVC)
    if (MSVC80 OR MSVC90 OR MSVC10 OR MSVC11 OR MSVC12)
      add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
      add_definitions(-D_CRT_NONSTDC_NO_WARNING)
      add_definitions(-D_SCL_SECURE_NO_WARNINGS)

      set(LAZPERF_COMPILER_MSVC 1)
      if (MSVC12)
        set(LAZPERF_COMPILER_VC12 1)
      elseif (MSVC11)
        set(LAZPERF_COMPILER_VC11 1)
      elseif (MSVC10)
        set(LAZPERF_COMPILER_VC10 1)
      elseif (MSVC9)
        set(LAZPERF_COMPILER_VC9 1)
      elseif (MSVC8)
        set(LAZPERF_COMPILER_VC8 1)
      endif()
    endif()

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

  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LAZPERF_COMMON_CXX_FLAGS}")
    if (CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()

    set(LAZPERF_COMPILER_GCC 1)

  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LAZPERF_COMMON_CXX_FLAGS}") # enable Weverything on clang, since we're badass
    set(LAZPERF_COMPILER_CLANG 1)
  endif()

  # Compiler-specific C++11 activation.
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	  execute_process(
		  COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
	  if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
		  message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
	  endif ()
  elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  else ()
	  message(FATAL_ERROR "Your C++ compiler does not support C++11.")
  endif ()
endif(WIN32)

if (APPLE)
  set(SO_EXT dylib)
  set(CMAKE_FIND_FRAMEWORK "LAST")
elseif(WIN32)
  set(SO_EXT dll)
else()
  set(SO_EXT so)
endif(APPLE)

# setup dependent libraries
if(WIN32 AND MSVC)
	message(STATUS "Setting windows specific Boost options.")
	set(BOOST_ROOT "C:/local/")
	set(BOOST_USE_STATIC_LIBS ON)
	set(BOOST_USE_MULTITHREADED ON)
else(WIN32 AND MSVC)
	set(BOOST_USE_STATIC_LIBS OFF)
	set(BOOST_USE_MULTITHREADED ON)
	set(BOOST_USE_STATIC_RUNTIME OFF)
endif(WIN32 AND MSVC)

set(LAZPERF_DEPENDENT_LIBRARIES "")

find_package(Boost "1.55.0" COMPONENTS unit_test_framework)
if(NOT Boost_FOUND)
	message(STATUS "Boost was not found, tests will not be available")
	set(WITH_TESTS false)
else()

	include_directories(${Boost_INCLUDE_DIRS})

	# Make sure compilers can find other variants of the libraries (libxxx), also don't explicitly add libraries, just the
	# path should suffice and auto-linking should take over
	if(WIN32 AND MSVC)
		link_directories(${Boost_LIBRARY_DIRS})
	else()
		set(LAZPERF_DEPENDENT_LIBRARIES ${Boost_LIBRARIES})
	endif()
endif(NOT Boost_FOUND)


# look for laszip
find_package(LASzip "2.0.1")
if(NOT LASZIP_FOUND)
	message(STATUS "LASzip >= 2.0.1 was not found, comparison benchmarks will be be available")
endif()

# if we have an emscripten build, make sure the emscripten tag is set
if(EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DEMSCRIPTEN_BUILD")
endif()


add_subdirectory(examples)
add_subdirectory(benchmarks)

if (EMSCRIPTEN)
	add_subdirectory(emscripten)
endif()

if(WITH_TESTS)
	if(NOT EMSCRIPTEN)
		enable_testing()
		add_subdirectory(test)
	endif()
endif()

if (WIN32 AND NOT CYGWIN)
  set (DEF_INSTALL_CMAKE_DIR cmake)
else ()
  set (DEF_INSTALL_CMAKE_DIR lib/cmake/lazperf)
endif ()
set (INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

configure_file (lazperf-config.cmake.in "lazperf-config.cmake" @ONLY)
configure_file (lazperf-configVersion.cmake.in "lazperf-configVersion.cmake" @ONLY)

# Install the lazperf-config.cmake and lazperf-configVersion.cmake
install (FILES "lazperf-config.cmake" "lazperf-configVersion.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

install(DIRECTORY laz-perf DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

