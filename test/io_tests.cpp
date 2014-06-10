/*
===============================================================================

  FILE:  io_tests.cpp
  
  CONTENTS:
    Factory to create dynamic compressors and decompressors

  PROGRAMMERS:

    uday.karan@gmail.com - Hobu, Inc.
  
  COPYRIGHT:

    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
===============================================================================
*/

#include <boost/test/unit_test.hpp>

#include "io.hpp"
#include "streams.hpp"

#include "reader.hpp"

BOOST_AUTO_TEST_SUITE(tlaz_io_tests)


BOOST_AUTO_TEST_CASE(io_structs_are_of_correct_size) {
	using namespace laszip::io;

	BOOST_CHECK_EQUAL(sizeof(header), 227);
}

BOOST_AUTO_TEST_CASE(can_report_invalid_magic) {
	using namespace laszip;

	{
		std::ifstream file("test/raw-sets/point10-1.las.raw", std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};

		BOOST_CHECK_THROW(func(), invalid_magic);

		file.close();
	}
}

BOOST_AUTO_TEST_CASE(can_check_for_no_compression) {
	using namespace laszip;

	{
		std::ifstream file("test/raw-sets/point10.las", std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};
		BOOST_CHECK_THROW(func(), not_compressed);
		file.close();
	}
}

BOOST_AUTO_TEST_CASE(doesnt_throw_any_errors_for_valid_laz) {
	using namespace laszip;
	{
		std::ifstream file("test/raw-sets/point10.las.laz", std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};

		BOOST_CHECK_NO_THROW(func());
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
		std::ifstream file("test/raw-sets/point10.las.laz", std::ios::binary);
		io::reader::file f(file);
		auto header = f.get_header();

		BOOST_CHECK_EQUAL(header.version.major, 1);
		BOOST_CHECK_EQUAL(header.version.minor, 2);

		BOOST_CHECK_EQUAL(header.creation.day, 113);
		BOOST_CHECK_EQUAL(header.creation.year, 2014);

		BOOST_CHECK_EQUAL(header.header_size, 227);
		BOOST_CHECK_EQUAL(header.point_offset, 1301u);

		BOOST_CHECK_EQUAL(header.vlr_count, 5u);

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

		BOOST_CHECK_EQUAL(header.point_count, 1065u);
	}
}

BOOST_AUTO_TEST_CASE(parses_laszip_vlr_correctly) {
	using namespace laszip;

	{
		std::ifstream file("test/raw-sets/point10.las.laz");
		io::reader::file f(file);
		auto vlr = f.get_laz_vlr();

		BOOST_CHECK_EQUAL(vlr.compressor, 2);
		BOOST_CHECK_EQUAL(vlr.coder, 0);
		
		BOOST_CHECK_EQUAL(vlr.version.major, 2);
		BOOST_CHECK_EQUAL(vlr.version.minor, 2);
		BOOST_CHECK_EQUAL(vlr.version.revision, 0);

		BOOST_CHECK_EQUAL(vlr.options, 0u);
		BOOST_CHECK_EQUAL(vlr.chunk_size, 50000u);

		BOOST_CHECK_EQUAL(vlr.num_points, -1);
		BOOST_CHECK_EQUAL(vlr.num_bytes, -1);

		BOOST_CHECK_EQUAL(vlr.items.size(), 1u);
		BOOST_CHECK_EQUAL(vlr.items[0].type, 6);
		BOOST_CHECK_EQUAL(vlr.items[0].size, 20);
		BOOST_CHECK_EQUAL(vlr.items[0].version, 2);
	}
}

BOOST_AUTO_TEST_CASE(decodes_single_chunk_files_correctly) {
	using namespace laszip;
	using namespace laszip::formats;

	{
		std::ifstream file("test/raw-sets/point10.las.laz");
		io::reader::file f(file);
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
		auto func = []() {
			std::ifstream file("test/raw-sets/autzen.laz");
			io::reader::file f(file);
		};

		BOOST_CHECK_NO_THROW(func());
	}
}

BOOST_AUTO_TEST_CASE(can_decode_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.laz");
	checkExists("test/raw-sets/autzen.las");

	{
		std::ifstream file("test/raw-sets/autzen.laz");
		io::reader::file f(file);
		reader fin("test/raw-sets/autzen.las");

		size_t pointCount = f.get_header().point_count;

		BOOST_CHECK_EQUAL(pointCount, fin.count_);

		struct pnt {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		for (size_t i = 0 ; i < pointCount ; i ++) {
			pnt p1, p2;

			f.readPoint((char*)&p1);
			fin.record((char*)&p2);

			// Make sure the points match
			{
				const las::point10& p = p2.p;
				const las::point10& pout = p1.p;

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

			// Make sure the gps time match
			BOOST_CHECK_EQUAL(p1.t.value, p2.t.value);

			// Make sure the colors match
			BOOST_CHECK_EQUAL(p1.c.r, p2.c.r);
			BOOST_CHECK_EQUAL(p1.c.g, p2.c.g);
			BOOST_CHECK_EQUAL(p1.c.b, p2.c.b);
		}
	}
}

BOOST_AUTO_TEST_CASE(can_encode_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.laz");
	checkExists("test/raw-sets/autzen.las");

	// write stuff to a temp file

	{
		// this is the format the autzen has points in
		struct point {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		factory::record_schema schema;

		// make schema
		schema
			(factory::record_item::POINT10)
			(factory::record_item::GPSTIME)
			(factory::record_item::RGB12);

		io::writer::file f("/tmp/autzen.laz", schema,
				io::writer::config(vector3<double>(0.01, 0.01, 0.01),
								   vector3<double>(0.0, 0.0, 0.0)));

		reader fin("test/raw-sets/autzen.las");

		size_t pointCount = fin.count_;
		point p;
		for (size_t i = 0 ; i < pointCount ; i ++) {
			fin.record((char*)&p);
			f.writePoint((char*)&p);
		}

		f.close();
	}
}


BOOST_AUTO_TEST_CASE(compression_decompression_is_symmetric) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.las");
	{
		// this is the format the autzen has points in
		struct point {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		factory::record_schema schema;

		// make schema
		schema
			(factory::record_item::POINT10)
			(factory::record_item::GPSTIME)
			(factory::record_item::RGB12);

		io::writer::file f("/tmp/autzen.laz", schema,
				io::writer::config(vector3<double>(0.01, 0.01, 0.01),
								   vector3<double>(0.0, 0.0, 0.0)));

		reader fin("test/raw-sets/autzen.las");

		size_t pointCount = fin.count_;
		point p;
		for (size_t i = 0 ; i < pointCount ; i ++) {
			fin.record((char*)&p);
			f.writePoint((char*)&p);
		}

		f.close();
	}

	// Now read that back and make sure points match
	{
		// this is the format the autzen has points in
		struct point {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		factory::record_schema schema;

		// make schema
		schema
			(factory::record_item::POINT10)
			(factory::record_item::GPSTIME)
			(factory::record_item::RGB12);

		std::ifstream file("/tmp/autzen.laz");
		io::reader::file f(file);
		reader fin("test/raw-sets/autzen.las");

		size_t pointCount = fin.count_;
		point p1, p2;
		for (size_t i = 0 ; i < pointCount ; i ++) {
			//std::cout << "# " << i << std::endl;
			fin.record((char*)&p1);
			f.readPoint((char*)&p2);

			// Make sure the points match
			{
				const las::point10& p = p2.p;
				const las::point10& pout = p1.p;

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

			// Make sure the gps time match
			BOOST_CHECK_EQUAL(p1.t.value, p2.t.value);

			// Make sure the colors match
			BOOST_CHECK_EQUAL(p1.c.r, p2.c.r);
			BOOST_CHECK_EQUAL(p1.c.g, p2.c.g);
			BOOST_CHECK_EQUAL(p1.c.b, p2.c.b);
		}

		file.close();
	}
}

BOOST_AUTO_TEST_CASE(can_decode_large_files_from_memory) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists("test/raw-sets/autzen.laz");
	checkExists("test/raw-sets/autzen.las");

	{
		std::ifstream file("test/raw-sets/autzen.laz");
		BOOST_CHECK_EQUAL(file.good(), true);

		file.seekg(0, std::ios::end);
		std::streamsize file_size = file.tellg();
		file.seekg(0);

		char *buf = (char *)malloc(file_size);
		file.read(buf, file_size);
		BOOST_CHECK_EQUAL(file.gcount(), file_size);
		file.close();

		streams::memory_stream ms(buf, file_size);

		io::reader::basic_file<streams::memory_stream> f(ms);
		reader fin("test/raw-sets/autzen.las");

		size_t pointCount = f.get_header().point_count;

		BOOST_CHECK_EQUAL(pointCount, fin.count_);

		struct pnt {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		for (size_t i = 0 ; i < pointCount ; i ++) {
			pnt p1, p2;

			f.readPoint((char*)&p1);
			fin.record((char*)&p2);

			// Make sure the points match
			{
				const las::point10& p = p2.p;
				const las::point10& pout = p1.p;

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

			// Make sure the gps time match
			BOOST_CHECK_EQUAL(p1.t.value, p2.t.value);

			// Make sure the colors match
			BOOST_CHECK_EQUAL(p1.c.r, p2.c.r);
			BOOST_CHECK_EQUAL(p1.c.g, p2.c.g);
			BOOST_CHECK_EQUAL(p1.c.b, p2.c.b);
		}

		free(buf);
	}
}

BOOST_AUTO_TEST_SUITE_END()
