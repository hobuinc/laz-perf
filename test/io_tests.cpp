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

#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "test_main.hpp"

#include <laz-perf/io.hpp>
#include <laz-perf/streams.hpp>

#include <cstdio>
#include "reader.hpp"
#include <stdio.h>

std::string makeTempFileName()
{
#ifdef _WIN32
    char *fnTemplate = "fnXXXXXX";
    char name[9];
	strcpy_s(name, sizeof(name), fnTemplate);
    size_t size = strnlen(name, 9) + 1;
    _mktemp_s(name, size);
	char path[MAX_PATH];
	GetTempPath(MAX_PATH, path);

    return std::string(path) + std::string(name, 8);
#else
    char name[L_tmpnam];
    std::tmpnam(name);

    return std::string(name);
#endif
}

TEST(io_tests, io_structs_are_of_correct_size) {
	using namespace laszip::io;

	EXPECT_EQ(sizeof(header), 227u);
}

TEST(io_tests, can_report_invalid_magic) {
	using namespace laszip;

	{
		std::ifstream file(testFile("point10-1.las.raw"), std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};

		EXPECT_THROW(func(), invalid_magic);

		file.close();
	}
}

TEST(io_tests, can_check_for_no_compression) {
	using namespace laszip;

	{
		std::ifstream file(testFile("point10.las"), std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};
		EXPECT_THROW(func(), not_compressed);
		file.close();
	}
}

TEST(io_tests, doesnt_throw_any_errors_for_valid_laz) {
	using namespace laszip;
	{
		std::ifstream file(testFile("point10.las.laz"), std::ios::binary);
		auto func = [&file]() {
			io::reader::file f(file);
		};

		EXPECT_NO_THROW(func());
	}
}

void dumpBytes(const char* b, size_t len) {
	for (size_t i = 0 ; i < len ; i ++) {
		printf("%02X ", (unsigned char)b[i]);

		if ((i+1) % 16 == 0)
			printf("\n");
	}
}

TEST(io_tests, parses_header_correctly) {
	using namespace laszip;

	{
		std::ifstream file(testFile("point10.las.laz"), std::ios::binary);
		io::reader::file f(file);
		auto header = f.get_header();

		EXPECT_EQ(header.version.major, 1);
		EXPECT_EQ(header.version.minor, 2);

		EXPECT_EQ(header.creation.day, 113);
		EXPECT_EQ(header.creation.year, 2014);

		EXPECT_EQ(header.header_size, 227);
		EXPECT_EQ(header.point_offset, 1301u);

		EXPECT_EQ(header.vlr_count, 5u);

		EXPECT_EQ(header.point_format_id, 0);
		EXPECT_EQ(header.point_record_length, 20);

		EXPECT_DOUBLE_EQ(header.scale.x, 0.01);
		EXPECT_DOUBLE_EQ(header.scale.y, 0.01);
		EXPECT_DOUBLE_EQ(header.scale.z, 0.01);

		EXPECT_DOUBLE_EQ(header.offset.x, 0.0);
		EXPECT_DOUBLE_EQ(header.offset.y, 0.0);
		EXPECT_DOUBLE_EQ(header.offset.z, 0.0);

		EXPECT_DOUBLE_EQ(header.min.x, 493994.87);
		EXPECT_DOUBLE_EQ(header.min.y, 4877429.62);
		EXPECT_DOUBLE_EQ(header.min.z, 123.93);

		EXPECT_DOUBLE_EQ(header.max.x, 494993.68);
		EXPECT_DOUBLE_EQ(header.max.y, 4878817.02);
		EXPECT_DOUBLE_EQ(header.max.z, 178.73);
/**
		EXPECT_DOUBLE_EQ(header.scale.x, 0.01, 0.0001);
		EXPECT_DOUBLE_EQ(header.scale.y, 0.01, 0.0001);
		EXPECT_DOUBLE_EQ(header.scale.z, 0.01, 0.0001);

		EXPECT_DOUBLE_EQ(header.offset.x, 0.0, 0.0001);
		EXPECT_DOUBLE_EQ(header.offset.y, 0.0, 0.0001);
		EXPECT_DOUBLE_EQ(header.offset.z, 0.0, 0.0001);

		EXPECT_DOUBLE_EQ(header.min.x, 493994.87, 0.0001);
		EXPECT_DOUBLE_EQ(header.min.y, 4877429.62, 0.0001);
		EXPECT_DOUBLE_EQ(header.min.z, 123.93, 0.0001);

		EXPECT_DOUBLE_EQ(header.max.x, 494993.68, 0.0001);
		EXPECT_DOUBLE_EQ(header.max.y, 4878817.02, 0.0001);
		EXPECT_DOUBLE_EQ(header.max.z, 178.73, 0.0001);
**/

		EXPECT_EQ(header.point_count, 1065u);
	}
}

TEST(io_tests, parses_laszip_vlr_correctly) {
	using namespace laszip;

	{
		std::ifstream file(testFile("point10.las.laz"));
		io::reader::file f(file);
		auto vlr = f.get_laz_vlr();

		EXPECT_EQ(vlr.compressor, 2);
		EXPECT_EQ(vlr.coder, 0);

		EXPECT_EQ(vlr.version.major, 2);
		EXPECT_EQ(vlr.version.minor, 2);
		EXPECT_EQ(vlr.version.revision, 0);

		EXPECT_EQ(vlr.options, 0u);
		EXPECT_EQ(vlr.chunk_size, 50000u);

		EXPECT_EQ(vlr.num_points, -1);
		EXPECT_EQ(vlr.num_bytes, -1);

		EXPECT_EQ(vlr.num_items, 1u);
		EXPECT_EQ(vlr.items[0].type, 6);
		EXPECT_EQ(vlr.items[0].size, 20);
		EXPECT_EQ(vlr.items[0].version, 2);
	}
}

TEST(io_tests, decodes_single_chunk_files_correctly) {
	using namespace laszip;
	using namespace laszip::formats;

	{
		std::ifstream file(testFile("point10.las.laz"), std::ios::binary);
		io::reader::file f(file);
		std::ifstream fin(testFile("point10-1.las.raw"), std::ios::binary);

		if (!fin.good())
			FAIL() << "Raw LAS file not available.";

		if (!file.good())
			FAIL() << "LAZ file not available.";


		size_t pointCount = f.get_header().point_count;
		EXPECT_EQ(pointCount, 1065u);

		for (size_t i = 0 ; i < pointCount ; i ++) {

			las::point10 p, pout;

			fin.read((char*)&p, sizeof(p));
			f.readPoint((char*)&pout);

			EXPECT_EQ(p.x, pout.x);
			EXPECT_EQ(p.y, pout.y);
			EXPECT_EQ(p.z, pout.z);
			EXPECT_EQ(p.intensity, pout.intensity);
			EXPECT_EQ(p.return_number, pout.return_number);
			EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
			EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
			EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
			EXPECT_EQ(p.classification, pout.classification);
			EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
			EXPECT_EQ(p.user_data, pout.user_data);
			EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
		}
	}
}

void checkExists(const std::string& filename) {
	std::ifstream f(filename, std::ios::binary);
	if (!f.good()) {
		std::stringstream sstr;
		sstr << "Could not open test file: " << filename << ", did you run the download-test-sets.sh script yet?";
		FAIL() << sstr.str();
	}

	f.close();
}


TEST(io_tests, can_open_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists(testFile("autzen_trim.laz"));

	{
		auto func = []() {
			std::ifstream file(testFile("autzen_trim.laz"));
			io::reader::file f(file);
		};

		EXPECT_NO_THROW(func());
	}
}

TEST(io_tests, can_decode_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists(testFile("autzen_trim.laz"));
	checkExists(testFile("autzen_trim.las"));

	{
		std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
		io::reader::file f(file);
		reader fin(testFile("autzen_trim.las"));

		size_t pointCount = f.get_header().point_count;

		EXPECT_EQ(pointCount, fin.count_);
		EXPECT_EQ( fin.count_, 110000u);

		struct pnt {
			las::point10 p;
			las::gpstime t;
			las::rgb c;
		};

		for (size_t i = 0 ; i < pointCount ; i ++) {
			pnt p1, p2;

			fin.record((char*)&p2);
			f.readPoint((char*)&p1);

			// Make sure the points match
			{
				const las::point10& p = p2.p;
				const las::point10& pout = p1.p;

				EXPECT_EQ(p.x, pout.x);
				EXPECT_EQ(p.y, pout.y);
				EXPECT_EQ(p.z, pout.z);
				EXPECT_EQ(p.intensity, pout.intensity);
				EXPECT_EQ(p.return_number, pout.return_number);
				EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
				EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
				EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
				EXPECT_EQ(p.classification, pout.classification);
				EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
				EXPECT_EQ(p.user_data, pout.user_data);
				EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
			}

			// Make sure the gps time match
			EXPECT_EQ(p1.t.value, p2.t.value);

			// Make sure the colors match
			EXPECT_EQ(p1.c.r, p2.c.r);
			EXPECT_EQ(p1.c.g, p2.c.g);
			EXPECT_EQ(p1.c.b, p2.c.b);
		}
	}
}

TEST(io_tests, can_encode_large_files) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists(testFile("autzen_trim.laz"));
	checkExists(testFile("autzen_trim.las"));

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
		std::string fname = makeTempFileName();
		io::writer::file f(fname, schema,
				io::writer::config(vector3<double>(0.01, 0.01, 0.01),
								   vector3<double>(0.0, 0.0, 0.0)));

		reader fin(testFile("autzen_trim.las"));

		size_t pointCount = fin.count_;
		point p;
		for (size_t i = 0 ; i < pointCount ; i ++) {
			fin.record((char*)&p);
			f.writePoint((char*)&p);
		}

		f.close();
	}
}


TEST(io_tests, compression_decompression_is_symmetric) {
	using namespace laszip;
	using namespace laszip::formats;

    std::string fname = makeTempFileName();

	checkExists(testFile("autzen_trim.las"));
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


		io::writer::file f(fname, schema,
				io::writer::config(vector3<double>(0.01, 0.01, 0.01),
								   vector3<double>(0.0, 0.0, 0.0)));

		reader fin(testFile("autzen_trim.las"));

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

		std::ifstream file(fname, std::ios::binary);
		io::reader::file f(file);
		reader fin(testFile("autzen_trim.las"));

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

				EXPECT_EQ(p.x, pout.x);
				EXPECT_EQ(p.y, pout.y);
				EXPECT_EQ(p.z, pout.z);
				EXPECT_EQ(p.intensity, pout.intensity);
				EXPECT_EQ(p.return_number, pout.return_number);
				EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
				EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
				EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
				EXPECT_EQ(p.classification, pout.classification);
				EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
				EXPECT_EQ(p.user_data, pout.user_data);
				EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
			}

			// Make sure the gps time match
			EXPECT_EQ(p1.t.value, p2.t.value);

			// Make sure the colors match
			EXPECT_EQ(p1.c.r, p2.c.r);
			EXPECT_EQ(p1.c.g, p2.c.g);
			EXPECT_EQ(p1.c.b, p2.c.b);
		}

		file.close();
	}
}

TEST(io_tests, can_decode_large_files_from_memory) {
	using namespace laszip;
	using namespace laszip::formats;

	checkExists(testFile("autzen_trim.laz"));
	checkExists(testFile("autzen_trim.las"));

	{
		std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
		EXPECT_EQ(file.good(), true);

		file.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize file_size = file.gcount();
		file.clear();   //  Since ignore will have set eof.
		file.seekg(0, std::ios_base::beg);

		char *buf = (char *)malloc(static_cast<size_t>(file_size));
		file.read(buf, file_size);
		EXPECT_EQ(file.gcount(), file_size);
		file.close();

		streams::memory_stream ms(buf, file_size);

		io::reader::basic_file<streams::memory_stream> f(ms);
		reader fin(testFile("autzen_trim.las"));

		size_t pointCount = f.get_header().point_count;

		EXPECT_EQ(pointCount, fin.count_);

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

				EXPECT_EQ(p.x, pout.x);
				EXPECT_EQ(p.y, pout.y);
				EXPECT_EQ(p.z, pout.z);
				EXPECT_EQ(p.intensity, pout.intensity);
				EXPECT_EQ(p.return_number, pout.return_number);
				EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
				EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
				EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
				EXPECT_EQ(p.classification, pout.classification);
				EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
				EXPECT_EQ(p.user_data, pout.user_data);
				EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
			}

			// Make sure the gps time match
			EXPECT_EQ(p1.t.value, p2.t.value);

			// Make sure the colors match
			EXPECT_EQ(p1.c.r, p2.c.r);
			EXPECT_EQ(p1.c.g, p2.c.g);
			EXPECT_EQ(p1.c.b, p2.c.b);
		}

		free(buf);
	}
}

TEST(io_tests, writes_bbox_to_header) {
	using namespace laszip;
	using namespace laszip::formats;

	factory::record_schema schema;
	schema(factory::record_item::POINT10);

	// First write a few points
	std::string filename(makeTempFileName());
	io::writer::file f(filename, schema,
			io::writer::config(vector3<double>(0.01, 0.01, 0.01),
							   vector3<double>(0.0, 0.0, 0.0)));
	las::point10 p1, p2;
	p1.x = 100; p2.x = 200;
	p1.y = -200; p2.y = -300;
	p1.z = 300; p2.z = -400;

	f.writePoint((char*)&p1);
	f.writePoint((char*)&p2);
	f.close();

	// Now check that the file has correct bounding box
  std::ifstream ifs(filename);
  io::reader::file reader(ifs);
  EXPECT_EQ(reader.get_header().min.x, 1.0);
  EXPECT_EQ(reader.get_header().max.x, 2.0);
  EXPECT_EQ(reader.get_header().min.y, -3.0);
  EXPECT_EQ(reader.get_header().max.y, -2.0);
  EXPECT_EQ(reader.get_header().min.z, -4.0);
  EXPECT_EQ(reader.get_header().max.z, 3.0);
}

TEST(io_tests, issue22)
{
    using namespace laszip;
    using namespace laszip::formats;

    std::string infile(testFile("classification.txt"));
    std::string tempfile(testFile("classification.laz"));

    std::ifstream ifs(infile);
    std::istream_iterator<int> it{ifs};
    std::vector<int> cls(it, std::istream_iterator<int>());

    factory::record_schema schema;
    schema(factory::record_item::POINT10);
    io::writer::file out(tempfile, schema,
            io::writer::config({0.01,0.01,0.01}, {0.0,0.0,0.0}));

    las::point10 p10;
    for(const int& cl : cls) {
        p10.classification = static_cast<unsigned char>(cl);
        out.writePoint((char *)&p10);
    }
    out.close();

    std::ifstream instream(tempfile, std::ios::binary);
    io::reader::file in(instream);

    EXPECT_EQ(in.get_header().point_count, cls.size());
    for (size_t i = 0; i < in.get_header().point_count; i++)
    {
        in.readPoint((char *)&p10);
        EXPECT_EQ(cls[i], p10.classification);
    }
    std::remove(tempfile.c_str());
}
