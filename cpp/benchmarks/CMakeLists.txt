include_directories(../laz-perf)

if (LASZIP_FOUND)
	include_directories(${LASZIP_INCLUDE_DIR})
	add_executable(laszip laszip.cpp)
	target_link_libraries(laszip ${ALL_LIBRARIES} ${LASZIP_LIBRARY})
endif()
