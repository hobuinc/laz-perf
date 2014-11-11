include_directories(../tlaz)

set(TLAZ_TEST_SOURCES test_main.cpp tlaz_tests.cpp io_tests.cpp stream_tests.cpp)

set(ALL_LIBRARIES ${TLAZ_DEPENDENT_LIBRARIES})

add_executable(tlaz_tests ${TLAZ_TEST_SOURCES})
target_link_libraries(tlaz_tests ${ALL_LIBRARIES})

add_test(NAME packers_are_symmetric
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/packers_are_symmetric)
add_test(NAME packers_canpack_gpstime
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/packers_canpack_gpstime)
add_test(NAME packers_canpack_rgb
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/packers_canpack_rgb)
add_test(NAME works_with_fields
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/works_with_fields)
add_test(NAME works_with_one_field
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/works_with_one_field)
add_test(NAME works_with_all_kinds_of_fields
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/works_with_all_kinds_of_fields)
add_test(NAME correctly_packs_unpacks_point10
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/correctly_packs_unpacks_point10)
add_test(NAME point10_enc_dec_is_sym
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/point10_enc_dec_is_sym)
add_test(NAME can_decode_laszip_buffer
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_decode_laszip_buffer)
add_test(NAME binary_matches_laszip
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/binary_matches_laszip)
add_test(NAME dynamic_compressor_works
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/dynamic_compressor_works)
add_test(NAME dynamic_can_do_blind_compression
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/dynamic_can_do_blind_compression)
add_test(NAME can_compress_decompress_gpstime
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_gpstime)
add_test(NAME can_compress_decompress_random_gpstime
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_random_gpstime)
add_test(NAME can_compress_decompress_rgb
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_rgb)
add_test(NAME can_compress_decompress_rgb_single_channel
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_rgb_single_channel)
add_test(NAME can_compress_decompress_real_gpstime
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_real_gpstime)
add_test(NAME can_compress_decompress_real_color
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_compress_decompress_real_color)
add_test(NAME las_structs_are_of_correct_size
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/las_structs_are_of_correct_size)
add_test(NAME can_encode_match_laszip_point10time
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_encode_match_laszip_point10time)
add_test(NAME can_encode_match_laszip_point10color
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_encode_match_laszip_point10color)
add_test(NAME can_encode_match_laszip_point10timecolor
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/can_encode_match_laszip_point10timecolor)
add_test(NAME schema_to_point_format_works
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/schema_to_point_format_works)
add_test(NAME just_xyz_encodes_and_decodes
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_tests/just_xyz_encodes_and_decodes)

add_test(NAME streams_are_sane
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_stream_tests/streams_are_sane)
add_test(NAME io_structs_are_of_correct_size
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/io_structs_are_of_correct_size)
add_test(NAME can_report_invalid_magic
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_report_invalid_magic)
add_test(NAME can_check_for_no_compression
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_check_for_no_compression)
add_test(NAME doesnt_throw_any_errors_for_valid_laz
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/doesnt_throw_any_errors_for_valid_laz)
add_test(NAME parses_header_correctly
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/parses_header_correctly)
add_test(NAME parses_laszip_vlr_correctly
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/parses_laszip_vlr_correctly)
add_test(NAME decodes_single_chunk_files_correctly
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/decodes_single_chunk_files_correctly)
add_test(NAME can_open_large_files
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_open_large_files)
add_test(NAME can_decode_large_files
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_decode_large_files)
add_test(NAME can_encode_large_files
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_encode_large_files)
add_test(NAME compression_decompression_is_symmetric
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/compression_decompression_is_symmetric)
add_test(NAME can_decode_large_files_from_memory
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND tlaz_tests --run_test=tlaz_io_tests/can_decode_large_files_from_memory)
