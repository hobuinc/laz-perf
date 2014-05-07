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

BOOST_AUTO_TEST_CASE(decodes_single_chunk_files_correctly) {
	using namespace laszip;
	using namespace laszip::formats;

	{
		io::file f("test/raw-sets/point10.las.laz");
		std::ifstream fin("test/raw-sets/point10-1.las.raw", std::ios::binary);

		if (!fin.good())
			BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

		size_t pointCount = f.get_header().point_count;

		for (size_t i = 0 ; i < pointCount ; i ++) {

			las::point10 p, pout;

			fin.read((char*)&p, sizeof(p));
			f.readPoint((char*)&pout);

			BOOST_CHECK_EQUAL(p.x, pout.x);
			BOOST_CHECK_EQUAL(p.y, pout.y);
			BOOST_CHECK_EQUAL(p.z, pout.z);
			BOOST_CHECK_EQUAL(p.intensity, pout.intensity);
			BOOST_CHECK_EQUAL(p.return_number, pout.return_number); 
			BOOST_CHECK_EQUAL(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
			BOOST_CHECK_EQUAL(p.scan_direction_flag, pout.scan_direction_flag);
			BOOST_CHECK_EQUAL(p.edge_of_flight_line, pout.edge_of_flight_line);
			BOOST_CHECK_EQUAL(p.classification, pout.classification);
			BOOST_CHECK_EQUAL(p.scan_angle_rank, pout.scan_angle_rank);
			BOOST_CHECK_EQUAL(p.user_data, pout.user_data);
			BOOST_CHECK_EQUAL(p.point_source_ID, pout.point_source_ID);
		}
	}
}

void checkExists(const std::string& filename) {
	std::ifstream f(filename, std::ios::binary);
	if (!f.good()) {
		std::stringstream sstr;
		sstr << "Could not open test file: " << filename << ", did you run the download-test-sets.sh script yet?";
		BOOST_FAIL(sstr.str());
	}

	f.close();
}


BOOST_AUTO_TEST_CASE(can_open_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.laz");

	{
		io::file f;

		BOOST_CHECK_NO_THROW(f.open("test/raw-sets/autzen.laz"));
	}
}

BOOST_AUTO_TEST_CASE(can_decode_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.laz");

	{
		io::file f("test/raw-sets/autzen.laz");

		size_t pointCount = f.get_header().point_count;

		for (size_t i = 0 ; i < pointCount ; i ++) {
			las::point10 p;

			f.readPoint((char*)&p);
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
