include_directories(../tlaz)

set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(simple simple.cpp)
target_link_libraries(simple ${ALL_LIBRARIES})

add_executable(point10 point10.cpp)
target_link_libraries(point10 ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(simple PROPERTIES SUFFIX .js)
	SET_TARGET_PROPERTIES(point10 PROPERTIES SUFFIX .js)
endif()
