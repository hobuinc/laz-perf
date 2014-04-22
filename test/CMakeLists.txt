include_directories(../tlaz)

set(TLAZ_TEST_SOURCES test_main.cpp tlaz_tests.cpp)

set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(tlaz_tests ${TLAZ_TEST_SOURCES})
target_link_libraries(tlaz_tests ${ALL_LIBRARIES})
