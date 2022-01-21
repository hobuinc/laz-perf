/*
===============================================================================

  FILE:  lazperf_tests.cpp

  CONTENTS:


  PROGRAMMERS:

    uday.karan@gmail.com - Hobu, Inc.

  COPYRIGHT:

    (c) 2007-2014, martin isenburg, rapidlasso - tools to catch reality
    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the Apache Public License 2.0 published by the Apache Software
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

#include <ctime>
#include <fstream>

#include <lazperf/encoder.hpp>
#include <lazperf/decoder.hpp>
#include <lazperf/las.hpp>

#include "reader.hpp"

using namespace lazperf;

TEST(lazperf_tests, packers_are_symmetric) {
	char buf[4];
	utils::pack(0xdeadbeaf, buf);
	unsigned int v = utils::unpack<unsigned int>(buf);
	EXPECT_EQ(v, 0xdeadbeaf);

	utils::pack(0xeadbeef, buf);
	v = utils::unpack<int>(buf);
	EXPECT_EQ(0xeadbeefu, v);

	utils::pack(0xbeef, buf);
	v = utils::unpack<unsigned short>(buf);
	EXPECT_EQ(0xbeefu, v);

	utils::pack(0xeef, buf);
	v = utils::unpack<short>(buf);
	EXPECT_EQ(0xeefu, v);
}


TEST(lazperf_tests, packers_canpack_gpstime) {

	{
		las::gpstime v((std::numeric_limits<int64_t>::max)());
		char buf[8];

		v.pack(buf);
		las::gpstime out(buf);

		EXPECT_EQ(v.value, out.value);
		EXPECT_EQ(std::equal(buf, buf + 8, (char *)&v.value), true);
	}

	{
		las::gpstime v((std::numeric_limits<int64_t>::min)());
		char buf[8];

		v.pack(buf);
		las::gpstime out(buf);

		EXPECT_EQ(v.value, out.value);
		EXPECT_EQ(std::equal(buf, buf + 8, (char *)&v.value), true);
	}
}

TEST(lazperf_tests, packers_canpack_rgb) {

	las::rgb c(1<<15, 1<<14, 1<<13);
	char buf[6];

	c.pack(buf);
	las::rgb out(buf);

	EXPECT_EQ(c.r, out.r);
	EXPECT_EQ(c.g, out.g);
	EXPECT_EQ(c.b, out.b);

	EXPECT_EQ(std::equal(buf, buf+2, (char*)&c.r), true);
	EXPECT_EQ(std::equal(buf+2, buf+4, (char*)&c.g), true);
	EXPECT_EQ(std::equal(buf+4, buf+6, (char*)&c.b), true);
}

TEST(lazperf_tests, las_structs_are_of_correct_size)
{
	EXPECT_EQ(sizeof(las::point10), 20u);
	EXPECT_EQ(sizeof(las::gpstime), 8u);
	EXPECT_EQ(sizeof(las::rgb), 6u);
}



TEST(lazperf_tests, correctly_packs_unpacks_point10) {

	for (int i = 0 ; i < 1000 ; i ++) {
		las::point10 p;

		p.x = i;
		p.y = i + 1000;
		p.z = i + 10000;

		p.intensity = (short)i + (1<14);
		p.return_number =  i & 0x7;
		p.number_of_returns_of_given_pulse = (i + 4) & 0x7;
		p.scan_angle_rank = i & 1;
		p.edge_of_flight_line = (i+1) & 1;
		p.classification = (i + (1 << 7)) % (1 << 8);
		p.scan_angle_rank = i % (1 << 7);
		p.user_data = (i + 64) % (1 << 7);
		p.point_source_ID = (short)i;

		char buf[sizeof(las::point10)];
		p.pack(buf);

		// Now unpack it back
		//
		las::point10 pout(buf);

		// Make things are still sane
		EXPECT_EQ(pout.x, p.x);
		EXPECT_EQ(pout.y, p.y);
		EXPECT_EQ(pout.z, p.z);

		EXPECT_EQ(pout.intensity, p.intensity);
		EXPECT_EQ(pout.return_number, p.return_number);
		EXPECT_EQ(pout.number_of_returns_of_given_pulse, p.number_of_returns_of_given_pulse);
		EXPECT_EQ(pout.scan_angle_rank, p.scan_angle_rank);
		EXPECT_EQ(pout.edge_of_flight_line, p.edge_of_flight_line);
		EXPECT_EQ(pout.classification, p.classification);
		EXPECT_EQ(pout.scan_angle_rank, p.scan_angle_rank);
		EXPECT_EQ(pout.user_data, p.user_data);
		EXPECT_EQ(pout.point_source_ID, p.point_source_ID);
	}
}

TEST(lazperf_tests, point10_enc_dec_is_sym)
{
    const int N = 100000;

    MemoryStream s;
    point_compressor_0 compressor(s.outCb(), 0);

    for (int i = 0 ; i < N; i ++) {
        las::point10 p;

        p.x = i;
        p.y = i + 1000;
        p.z = i + 10000;

        p.intensity = (i + (1<14)) % (1 << 16);
        p.return_number =  i & 0x7;
        p.number_of_returns_of_given_pulse = (i + 4) & 0x7;
        p.scan_direction_flag = i & 1;
        p.edge_of_flight_line = (i+1) & 1;
        p.classification = (i + (1 << 7)) % (1 << 8);
        p.scan_angle_rank = i % (1 << 7);
        p.user_data = (i + 64) % (1 << 7);
        p.point_source_ID = i % (1 << 16);

        char buf[sizeof(las::point10)];
        p.pack(buf);
        compressor.compress(buf);
    }
    compressor.done();

    point_decompressor_0 decompressor(s.inCb(), 0);

    char buf[sizeof(las::point10)];
    for (int i = 0 ; i < N ; i ++) {
        decompressor.decompress((char *)buf);

        las::point10 p(buf);

        EXPECT_EQ(p.x, i);
        EXPECT_EQ(p.y, i + 1000);
        EXPECT_EQ(p.z, i + 10000);

        EXPECT_EQ(p.intensity, (i + (1<14)) % (1 << 16));
        EXPECT_EQ(p.return_number,  i & 0x7);
        EXPECT_EQ(p.number_of_returns_of_given_pulse, (i + 4) & 0x7);
        EXPECT_EQ(p.scan_direction_flag, i & 1);
        EXPECT_EQ(p.edge_of_flight_line, (i+1) & 1);
        EXPECT_EQ(p.classification, (i + (1 << 7)) % (1 << 8));
        EXPECT_EQ(p.scan_angle_rank, i % (1 << 7));
        EXPECT_EQ(p.user_data, (i + 64) % (1 << 7));
        EXPECT_EQ(p.point_source_ID, i % (1 << 16));
    }
}

void printPoint(const lazperf::las::point10& p) {
	printf("x: %i, y: %i, z: %i, i: %u, rn: %i, nor: %i, sdf: %i, efl: %i, c: %i, "
		   "sar: %i, ud: %i, psid: %i\n",
		   p.x, p.y, p.z,
		   p.intensity, p.return_number, p.number_of_returns_of_given_pulse,
		   p.scan_direction_flag, p.edge_of_flight_line,
		   p.classification, p.scan_angle_rank, p.user_data, p.point_source_ID);
}

void printBytes(const unsigned char *bytes, size_t len) {
	for (size_t i = 0 ; i < len ; i ++) {
		printf("%02X ", bytes[i]);
		if ((i+1) % 16 == 0)
			printf("\n");
	}

	printf("\n");
}

TEST(lazperf_tests, can_compress_decompress_real_data) {

	std::ifstream f(testFile("point10-1.las.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAS file not available.";

	las::point10 pnt;

	MemoryStream s;
    point_compressor_0 compressor(s.outCb(), 0);

    std::vector<las::point10> points;
	while(!f.eof()) {
		f.read((char *)&pnt, sizeof(pnt));
		compressor.compress((const char*)&pnt);

		points.push_back(pnt);
	}
	compressor.done();

	f.close();

    point_decompressor_0 decompressor(s.inCb(), 0);

	for (size_t i = 0 ; i < points.size() ; i ++) {
		char buf[sizeof(las::point10)];
		las::point10 pout;
		decompressor.decompress((char *)buf);

		pout.unpack(buf);

		// Make sure all fields match
		las::point10& p = points[i];

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


TEST(lazperf_tests, can_decode_laszip_buffer) {

	std::ifstream f(testFile("point10-1.las.laz.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAZ file not available.";

	f.seekg(0, std::ios::end);
	size_t fileSize = (size_t)f.tellg();
	f.seekg(0);

    MemoryStream s;

	// Read all of the file data in one go
	s.buf.resize(fileSize);
	f.read((char*)&s.buf[0], fileSize);

	f.close();

	// start decoding our data, while we do that open the raw las file for comparison

    point_decompressor_0 decompressor(s.inCb(), 0);

	// open raw las point stream
	std::ifstream fin(testFile("point10-1.las.raw"), std::ios::binary);
	if (!fin.good())
		FAIL() << "Raw LAS file not available.";

	fin.seekg(0, std::ios::end);
	size_t count = (size_t)fin.tellg() / sizeof(las::point10);
	fin.seekg(0);

	while(count --) {
		las::point10 p, pout;

		fin.read((char *)&p, sizeof(p));

		// decompress record
		//
		decompressor.decompress((char*)&pout);

		// make sure they match
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

	fin.close();
}

void matchSets(const std::string& lasRaw, const std::string& lazRaw)
{
	std::ifstream f(lasRaw, std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAS file not available.";

	las::point10 pnt;
	f.seekg(0, std::ios::end);
	size_t count = (size_t)f.tellg() / sizeof(las::point10);
	f.seekg(0);

    MemoryStream s;

    point_compressor_0 comp(s.outCb(), 0);

	while(count --) {
		f.read((char *)&pnt, sizeof(pnt));
		comp.compress((const char*)&pnt);
	}

	comp.done();
	f.close();


	// now open compressed data file
	std::ifstream fc(lazRaw, std::ios::binary);
	if (!fc.good())
		FAIL() << "Raw LAZ file not available.";

	// Make sure things match
	for (size_t i = 0 ; i < s.buf.size() && !fc.eof() ; i ++) {
		EXPECT_EQ(s.buf[i], fc.get());
	}

	fc.close();
}

TEST(lazperf_tests, binary_matches_laszip) {
	matchSets(testFile("point10-1.las.raw"), testFile("point10-1.las.laz.raw"));
}


TEST(lazperf_tests, dynamic_compressor_works) {

	const std::string lasRaw = testFile("point10-1.las.raw");
	const std::string lazRaw = testFile("point10-1.las.laz.raw");

    MemoryStream s;

	las_compressor::ptr pcompressor = build_las_compressor(s.outCb(), 0);

	std::ifstream f(lasRaw, std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAS file not available.";

	las::point10 pnt;
	f.seekg(0, std::ios::end);
	size_t count = (size_t)f.tellg() / sizeof(las::point10);
	f.seekg(0);

	while(count --) {
		f.read((char *)&pnt, sizeof(pnt));
		pcompressor->compress((char *)&pnt);
	}

	// flush encoder
    pcompressor->done();
	f.close();


	// now open compressed data file
	std::ifstream fc(lazRaw, std::ios::binary);
	if (!fc.good())
		FAIL() << "Raw LAZ file not available.";

	// Make sure things match
	for (size_t i = 0 ; i < s.buf.size() && !fc.eof() ; i ++) {
		EXPECT_EQ(s.buf[i], fc.get());
	}

	fc.close();
}


TEST(lazperf_tests, dynamic_decompressor_can_decode_laszip_buffer) {

	std::ifstream f(testFile("point10-1.las.laz.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAZ file not available.";

	f.seekg(0, std::ios::end);
	size_t fileSize = (size_t)f.tellg();
	f.seekg(0);

    MemoryStream s;

	// Read all of the file data in one go
	s.buf.resize(fileSize);
	f.read((char*)&s.buf[0], fileSize);

	f.close();

	// start decoding our data, while we do that open the raw las file for comparison

    las_decompressor::ptr pdecomp = build_las_decompressor(s.inCb(), 0);

	// open raw las point stream
	std::ifstream fin(testFile("point10-1.las.raw"), std::ios::binary);
	if (!fin.good())
		FAIL() << "Raw LAS file not available.";

	fin.seekg(0, std::ios::end);
	size_t count = (size_t)fin.tellg() / sizeof(las::point10);
	fin.seekg(0);

	while(count --) {
		las::point10 p, pout;

		fin.read((char *)&p, sizeof(p));

		// decompress record
		//
		pdecomp->decompress((char*)&pout);

		// make sure they match
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

	fin.close();
}

TEST(lazperf_tests, can_encode_match_laszip_point10time)
{
	test::reader laz(testFile("point-time.las.laz"));
    test::reader las(testFile("point-time.las"));

    MemoryStream s;

	point_compressor_1 comp(s.outCb());

	struct {
		las::point10 p;
		las::gpstime t;
	} p;

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);
		comp.compress((char*)&p);
	}
	comp.done();

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < s.buf.size(); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, can_encode_match_laszip_point10color)
{
	test::reader laz(testFile("point-color.las.laz"));
    test::reader las(testFile("point-color.las"));

    MemoryStream s;
    
    point_compressor_2 comp(s.outCb());

#pragma pack(push, 1)
	struct {
		las::point10 p;
		las::rgb c;
	} p;
#pragma pack(pop)

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);
//		std::cout << "i = " << i << ", c: " << p.c.r << ", " << p.c.g << ", " << p.c.b << std::endl;
		comp.compress((char*)&p);
	}
	comp.done();

//	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < (std::min)(30u, (unsigned int)s.buf.size()); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, can_encode_match_laszip_point10timecolor)
{
	test::reader laz(testFile("point-color-time.las.laz"));
    test::reader las(testFile("point-color-time.las"));


	MemoryStream s;
    point_compressor_3 comp(s.outCb());

#pragma pack(push, 1)
	struct {
		las::point10 p;
		las::gpstime t;
		las::rgb c;
	} p;
#pragma pack(pop)

	for (unsigned int i = 0 ; i < las.count_ ; i ++)
    {
		las.record((char*)&p);
		comp.compress((char*)&p);
	}
	comp.done();

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < (std::min)(30u, (unsigned int)s.buf.size()); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, point_10_intensity)
{
    MemoryStream s;

    point_compressor_0 comp(s.outCb());

	std::array<las::point10, 7> points;
	points[0].intensity = 257;
	points[1].intensity = 0;
	points[2].intensity = 0;
	points[3].intensity = 0;
	points[4].intensity = 257;
	points[5].intensity = 257;
	points[6].intensity = 514;

	for (const las::point10& point : points)
		comp.compress((const char*)&point);
	comp.done();

    point_decompressor_0 decomp(s.inCb());

	las::point10 decompressedPoint;
	for (const las::point10 &point : points)
	{
		decomp.decompress((char *)&decompressedPoint);
		EXPECT_EQ(point.intensity, decompressedPoint.intensity);
	}
}
