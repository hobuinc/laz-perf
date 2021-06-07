if (NOT ROOT_DIR)
    message(FATAL_ERROR "ROOT_DIR must be set in the top-level CMakeLists.txt")
endif()
set(LAZPERF_CMAKE_DIR ${ROOT_DIR}/cmake)
