include_directories(../tlaz)


set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(brute brute.cpp)
target_link_libraries(brute ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(brute PROPERTIES SUFFIX .js)
endif()
