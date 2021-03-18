include_directories(../laz-perf)


set(ALL_LIBRARIES ${LAZPERF_DEPENDENT_LIBRARIES})

#add_executable(brute_point10 brute_point10.cpp)
#target_link_libraries(brute_point10 ${ALL_LIBRARIES})

if (EMSCRIPTEN)
	SET_TARGET_PROPERTIES(brute PROPERTIES SUFFIX .js)
#	SET_TARGET_PROPERTIES(brute_point10 PROPERTIES SUFFIX .js)
endif()

if (LASZIP_FOUND)
	include_directories(${LASZIP_INCLUDE_DIR})
	add_executable(laszip laszip.cpp)
	target_link_libraries(laszip ${ALL_LIBRARIES} ${LASZIP_LIBRARY})
endif()
