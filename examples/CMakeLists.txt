include_directories(../tlaz)

set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(simple simple.cpp)
target_link_libraries(simple ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(simple PROPERTIES SUFFIX .js)
endif()
