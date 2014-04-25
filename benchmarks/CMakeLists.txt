include_directories(../tlaz)


set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(brute brute.cpp)
add_executable(brute_point10 brute_point10.cpp)
target_link_libraries(brute ${ALL_LIBRARIES})
target_link_libraries(brute_point10 ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(brute PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(brute_point10 PROPERTIES SUFFIX .js)
endif()
