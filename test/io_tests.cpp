// io_tests.cpp
// IO Tests
//
#include <boost/test/unit_test.hpp>

#include "io.hpp"

BOOST_AUTO_TEST_SUITE(tlaz_io_tests)


BOOST_AUTO_TEST_CASE(can_report_invalid_files) {
	using namespace laszip;

	io::file f;
	BOOST_CHECK_THROW(f.open("test/raw-sets/point10-1-not-found.las.laz"), file_not_found);
}

BOOST_AUTO_TEST_CASE(can_report_invalid_magic) {
	using namespace laszip;

	{
		io::file f;
		BOOST_CHECK_THROW(f.open("test/raw-sets/point10-1.las.raw"), invalid_magic);
	}
}

BOOST_AUTO_TEST_CASE(can_check_for_no_compression) {
	using namespace laszip;

	{
		io::file f;
		BOOST_CHECK_THROW(f.open("test/raw-sets/point10.las"), not_compressed);
	}
}

BOOST_AUTO_TEST_CASE(doesnt_throw_any_errors_for_valid_laz) {
	using namespace laszip;
	{
		io::file f;
		BOOST_CHECK_NO_THROW(f.open("test/raw-sets/point10.las.laz"));
	}
}

BOOST_AUTO_TEST_CASE(fails_for_headers_for_invalid_files) {
	using namespace laszip;

	{
		io::file f;
		BOOST_CHECK_THROW(f.get_header(), invalid_header_request);
	}
}

void dumpBytes(const char* b, size_t len) {
	for (size_t i = 0 ; i < len ; i ++) {
		printf("%02X ", (unsigned char)b[i]);

		if ((i+1) % 16 == 0)
			printf("\n");
	}
}

BOOST_AUTO_TEST_CASE(parses_header_correctly) {
	using namespace laszip;

	{
		io::file f("test/raw-sets/point10.las.laz");
		auto header = f.get_header();

		BOOST_CHECK_EQUAL(header.version.major, 1);
		BOOST_CHECK_EQUAL(header.version.minor, 2);

		BOOST_CHECK_EQUAL(header.creation.day, 113);
		BOOST_CHECK_EQUAL(header.creation.year, 2014);

		BOOST_CHECK_EQUAL(header.header_size, 227);
		BOOST_CHECK_EQUAL(header.point_offset, 1301);

		BOOST_CHECK_EQUAL(header.vlr_count, 5);

		BOOST_CHECK_EQUAL(header.point_format_id, 0);
		BOOST_CHECK_EQUAL(header.point_record_length, 20);

		BOOST_CHECK_CLOSE(header.scale.x, 0.01, 0.0001);
		BOOST_CHECK_CLOSE(header.scale.y, 0.01, 0.0001);
		BOOST_CHECK_CLOSE(header.scale.z, 0.01, 0.0001);

		BOOST_CHECK_CLOSE(header.offset.x, 0.0, 0.0001);
		BOOST_CHECK_CLOSE(header.offset.y, 0.0, 0.0001);
		BOOST_CHECK_CLOSE(header.offset.z, 0.0, 0.0001);

		BOOST_CHECK_CLOSE(header.min.x, 493994.87, 0.0001);
		BOOST_CHECK_CLOSE(header.min.y, 4877429.62, 0.0001);
		BOOST_CHECK_CLOSE(header.min.z, 123.93, 0.0001);

		BOOST_CHECK_CLOSE(header.max.x, 494993.68, 0.0001);
		BOOST_CHECK_CLOSE(header.max.y, 4878817.02, 0.0001);
		BOOST_CHECK_CLOSE(header.max.z, 178.73, 0.0001);

		BOOST_CHECK_EQUAL(header.point_count, 1065);
	}
}

BOOST_AUTO_TEST_CASE(parses_laszip_vlr_correctly) {
	using namespace laszip;

	{
		io::file f("test/raw-sets/point10.las.laz");
		auto vlr = f.get_laz_vlr();

		BOOST_CHECK_EQUAL(vlr.compressor, 2);
		BOOST_CHECK_EQUAL(vlr.coder, 0);
		
		BOOST_CHECK_EQUAL(vlr.version.major, 2);
		BOOST_CHECK_EQUAL(vlr.version.minor, 2);
		BOOST_CHECK_EQUAL(vlr.version.revision, 0);

		BOOST_CHECK_EQUAL(vlr.options, 0);
		BOOST_CHECK_EQUAL(vlr.chunk_size, 50000);

		BOOST_CHECK_EQUAL(vlr.num_points, -1);
		BOOST_CHECK_EQUAL(vlr.num_bytes, -1);

		BOOST_CHECK_EQUAL(vlr.items.size(), 1);
		BOOST_CHECK_EQUAL(vlr.items[0].type, 6);
		BOOST_CHECK_EQUAL(vlr.items[0].size, 20);
		BOOST_CHECK_EQUAL(vlr.items[0].version, 2);
	}
}


BOOST_AUTO_TEST_SUITE_END()
