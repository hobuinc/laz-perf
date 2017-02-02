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

#include <ctime>
#include <fstream>

#include <laz-perf/encoder.hpp>
#include <laz-perf/decoder.hpp>
#include <laz-perf/formats.hpp>
#include <laz-perf/las.hpp>
#include <laz-perf/factory.hpp>

#include "reader.hpp"

struct SuchStream {
	SuchStream() : buf(), idx(0) {}

	void putBytes(const unsigned char* b, size_t len) {
		while(len --) {
			buf.push_back(*b++);
		}
	}

	void putByte(const unsigned char b) {
		buf.push_back(b);
	}

	unsigned char getByte() {
		return buf[idx++];
	}

	void getBytes(unsigned char *b, int len) {
		for (int i = 0 ; i < len ; i ++) {
			b[i] = getByte();
		}
	}

	std::vector<unsigned char> buf;
	size_t idx;
};


TEST(lazperf_tests, packers_are_symmetric) {
	using namespace laszip::formats;

	char buf[4];
	packers<unsigned int>::pack(0xdeadbeaf, buf);
	unsigned int v = packers<unsigned int>::unpack(buf);
	EXPECT_EQ(v, 0xdeadbeaf);

	packers<int>::pack(0xeadbeef, buf);
	v = packers<int>::unpack(buf);
	EXPECT_EQ(0xeadbeefu, v);

	packers<unsigned short>::pack(0xbeef, buf);
	v = packers<unsigned short>::unpack(buf);
	EXPECT_EQ(0xbeefu, v);

	packers<short>::pack(0xeef, buf);
	v = packers<short>::unpack(buf);
	EXPECT_EQ(0xeefu, v);

	packers<unsigned char>::pack(0xf, buf);
	v = packers<unsigned char>::unpack(buf);
	EXPECT_EQ(0xfu, v);

	packers<char>::pack(0x7, buf);
	v = packers<char>::unpack(buf);
	EXPECT_EQ(0x7u, v);
}


TEST(lazperf_tests, packers_canpack_gpstime) {
	using namespace laszip::formats;

	{
		las::gpstime v(std::numeric_limits<int64_t>::max());
		char buf[8];

		packers<las::gpstime>::pack(v, buf);
		las::gpstime out = packers<las::gpstime>::unpack(buf);

		EXPECT_EQ(v.value, out.value);
		EXPECT_EQ(std::equal(buf, buf + 8, (char *)&v.value), true);
	}

	{
		las::gpstime v(std::numeric_limits<int64_t>::min());
		char buf[8];

		packers<las::gpstime>::pack(v, buf);
		las::gpstime out = packers<las::gpstime>::unpack(buf);

		EXPECT_EQ(v.value, out.value);
		EXPECT_EQ(std::equal(buf, buf + 8, (char *)&v.value), true);
	}
}

TEST(lazperf_tests, packers_canpack_rgb) {
	using namespace laszip::formats;

	las::rgb c(1<<15, 1<<14, 1<<13);
	char buf[6];

	packers<las::rgb>::pack(c, buf);
	las::rgb out = packers<las::rgb>::unpack(buf);

	EXPECT_EQ(c.r, out.r);
	EXPECT_EQ(c.g, out.g);
	EXPECT_EQ(c.b, out.b);

	EXPECT_EQ(std::equal(buf, buf+2, (char*)&c.r), true);
	EXPECT_EQ(std::equal(buf+2, buf+4, (char*)&c.g), true);
	EXPECT_EQ(std::equal(buf+4, buf+6, (char*)&c.b), true);
}

TEST(lazperf_tests, las_structs_are_of_correct_size) {
	using namespace laszip::formats;

	EXPECT_EQ(sizeof(las::point10), 20u);
	EXPECT_EQ(sizeof(las::gpstime), 8u);
	EXPECT_EQ(sizeof(las::rgb), 6u);
}

TEST(lazperf_tests, works_with_fields) {
	using namespace laszip;
	using namespace laszip::formats;

	struct {
		int a;
		short b;
		unsigned short c;
		unsigned int d;
	} data;

	record_compressor<
		field<int>,
		field<short>,
		field<unsigned short>,
		field<unsigned int> > compressor;

	SuchStream s;

	encoders::arithmetic<SuchStream> encoder(s);

	for (int i = 0 ; i < 1000; i ++) {
		data.a = i;
		data.b = (short)i + 10;
		data.c = (short)i + 40000;
		data.d = (unsigned int)i + (1 << 31);

		compressor.compressWith(encoder, (const char*)&data);
	}
	encoder.done();

	record_decompressor<
		field<int>,
		field<short>,
		field<unsigned short>,
		field<unsigned int> > decompressor;

	decoders::arithmetic<SuchStream> decoder(s);

	for (int i = 0 ; i < 10 ; i ++) {
		decompressor.decompressWith(decoder, (char *)&data);

		EXPECT_EQ(data.a, i);
		EXPECT_EQ(data.b, i+10);
		EXPECT_EQ(data.c, i+40000);
		EXPECT_EQ(data.d, (unsigned int)i+ (1<<31));
	}
}

TEST(lazperf_tests, works_with_one_field) {
	using namespace laszip;
	using namespace laszip::formats;

	struct {
		int a;
	} data;

	record_compressor<
		field<int> > compressor;

	SuchStream s;

	encoders::arithmetic<SuchStream> encoder(s);

	for (int i = 0 ; i < 1000; i ++) {
		data.a = i;
		compressor.compressWith(encoder, (const char*)&data);
	}
	encoder.done();

	record_decompressor<
		field<int> > decompressor;

	decoders::arithmetic<SuchStream> decoder(s);

	for (int i = 0 ; i < 1000 ; i ++) {
		decompressor.decompressWith(decoder, (char *)&data);
		EXPECT_EQ(data.a, i);
	}
}

TEST(lazperf_tests, works_with_all_kinds_of_fields) {
	using namespace laszip;
	using namespace laszip::formats;

	struct {
		int a;
		unsigned int ua;
		short b;
		unsigned short ub;
		char c;
		unsigned char uc;
	} data;

	record_compressor<
		field<int>,
		field<unsigned int>,
		field<short>,
		field<unsigned short>,
		field<char>,
		field<unsigned char>
	> compressor;

	SuchStream s;

	encoders::arithmetic<SuchStream> encoder(s);

	for (int i = 0 ; i < 1000; i ++) {
		data.a = i;
		data.ua = (unsigned int)i + (1<<31);
		data.b = (short)i;
		data.ub = (unsigned short)i + (1<<15);
		data.c = i % 128;
		data.uc = (unsigned char)(i % 128) + (1<<7);

		compressor.compressWith(encoder, (const char*)&data);
	}
	encoder.done();

	record_decompressor<
		field<int>,
		field<unsigned int>,
		field<short>,
		field<unsigned short>,
		field<char>,
		field<unsigned char>
	> decompressor;

	decoders::arithmetic<SuchStream> decoder(s);

	for (int i = 0 ; i < 1000 ; i ++) {
		decompressor.decompressWith(decoder, (char *)&data);
		EXPECT_EQ(data.a, i);
		EXPECT_EQ(data.ua, (unsigned int)i + (1<<31));
		EXPECT_EQ(data.b, i);
		EXPECT_EQ(data.ub, (unsigned short)i + (1<<15));
		EXPECT_EQ(data.c, i % 128);
		EXPECT_EQ(data.uc, (unsigned char)(i % 128) + (1<<7));
	}
}


TEST(lazperf_tests, correctly_packs_unpacks_point10) {
	using namespace laszip;
	using namespace laszip::formats;

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
		packers<las::point10>::pack(p, buf);

		// Now unpack it back
		//
		las::point10 pout = packers<las::point10>::unpack(buf);

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

TEST(lazperf_tests, point10_enc_dec_is_sym) {
	using namespace laszip;
	using namespace laszip::formats;

	record_compressor<
		field<las::point10>
	> compressor;

	SuchStream s;

	const int N = 100000;

	encoders::arithmetic<SuchStream> encoder(s);

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
		packers<las::point10>::pack(p, buf);

		compressor.compressWith(encoder, buf);
	}
	encoder.done();

	record_decompressor<
		field<las::point10>
	> decompressor;

	decoders::arithmetic<SuchStream> decoder(s);

	char buf[sizeof(las::point10)];
	for (int i = 0 ; i < N ; i ++) {
		decompressor.decompressWith(decoder, (char *)buf);

		las::point10 p = packers<las::point10>::unpack(buf);

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


void printPoint(const laszip::formats::las::point10& p) {
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
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f(testFile("point10-1.las.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAS file not available.";

	las::point10 pnt;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::point10>
	> comp;

	std::vector<las::point10> points;	// hopefully not too many points in our test case :)

	while(!f.eof()) {
		f.read((char *)&pnt, sizeof(pnt));
		comp.compressWith(encoder, (const char*)&pnt);

		points.push_back(pnt);
	}

	encoder.done();

	f.close();

	decoders::arithmetic<SuchStream> decoder(s);

	record_decompressor<
		field<las::point10>
	> decomp;

	for (size_t i = 0 ; i < points.size() ; i ++) {
		char buf[sizeof(las::point10)];
		las::point10 pout;
		decomp.decompressWith(decoder, (char *)buf);

		pout = packers<las::point10>::unpack(buf);

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
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f(testFile("point10-1.las.laz.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAZ file not available.";

	f.seekg(0, std::ios::end);
	size_t fileSize = (size_t)f.tellg();
	f.seekg(0);

	SuchStream s;

	// Read all of the file data in one go
	s.buf.resize(fileSize);
	f.read((char*)&s.buf[0], fileSize);

	f.close();

	// start decoding our data, while we do that open the raw las file for comparison

	decoders::arithmetic<SuchStream> dec(s);
	record_decompressor<
		field<las::point10>
	> decomp;

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
		decomp.decompressWith(dec, (char*)&pout);

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

void matchSets(const std::string& lasRaw, const std::string& lazRaw) {
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f(lasRaw, std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAS file not available.";

	las::point10 pnt;
	f.seekg(0, std::ios::end);
	size_t count = (size_t)f.tellg() / sizeof(las::point10);
	f.seekg(0);

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::point10>
	> comp;

	while(count --) {
		f.read((char *)&pnt, sizeof(pnt));
		comp.compressWith(encoder, (const char*)&pnt);
	}

	encoder.done();
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
	using namespace laszip;
	using namespace laszip::formats;

	const std::string lasRaw = testFile("point10-1.las.raw");
	const std::string lazRaw = testFile("point10-1.las.laz.raw");

	typedef encoders::arithmetic<SuchStream> Encoder;

	SuchStream s;
	Encoder encoder(s);

	auto compressor = new record_compressor<field<las::point10> >();

	dynamic_compressor::ptr pcompressor =
		make_dynamic_compressor(encoder, compressor);

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
	encoder.done();
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
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f(testFile("point10-1.las.laz.raw"), std::ios::binary);
	if (!f.good())
		FAIL() << "Raw LAZ file not available.";

	f.seekg(0, std::ios::end);
	size_t fileSize = (size_t)f.tellg();
	f.seekg(0);

	SuchStream s;

	// Read all of the file data in one go
	s.buf.resize(fileSize);
	f.read((char*)&s.buf[0], fileSize);

	f.close();

	// start decoding our data, while we do that open the raw las file for comparison

	typedef decoders::arithmetic<SuchStream> Decoder;
	auto decomp = new record_decompressor<field<las::point10> >();

	Decoder dec(s);
	dynamic_decompressor::ptr pdecomp = make_dynamic_decompressor(dec, decomp);

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

int64_t makegps(unsigned int upper, unsigned int lower) {
	int64_t u = upper,
			l = lower;
	return (u << 32) | l;
}

TEST(lazperf_tests, can_compress_decompress_gpstime) {
	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::gpstime>
	> comp;

	const int il = 31, jl = (1 << 16) - 1;

	for (size_t i = 0 ; i < il ; i ++) {
		for (size_t j = 0 ; j < jl ; j ++) {
			las::gpstime t(makegps((unsigned int)(i > 0 ? 1ll << i : 0), (unsigned int)((j << 16) + j)));
			comp.compressWith(encoder, (const char*)&t);
		}
	}
	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::gpstime>
	> decomp;

	for (size_t i = 0 ; i < il ; i ++) {
		for (size_t j = 0 ; j < jl ; j ++) {
			las::gpstime t(makegps((unsigned int)((i > 0 ? 1ll << i : 0)), (unsigned int)( (j << 16) + j)));
			las::gpstime out;
			decomp.decompressWith(decoder, (char *)&out);

			EXPECT_EQ(out.value, t.value);
		}
	}
}

TEST(lazperf_tests, can_compress_decompress_random_gpstime) {
	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::gpstime>
	> comp;

	const size_t S = 1000;

	srand((unsigned int)std::time(NULL));
	int rvalue = rand();
	std::vector<int64_t> vs(S);
	for (size_t i = 0 ; i < S ; i ++) {
		int64_t a = rvalue & 0xFFFF,
				b = rvalue & 0xFFFF,
				c = rvalue & 0xFFFF,
				d = rvalue & 0xFFFF;

		las::gpstime t((a << 48) | (b << 32) | (c << 16) | d);
		vs[i] = t.value;

		comp.compressWith(encoder, (const char*)&t);
	}
	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::gpstime>
	> decomp;

	for (size_t i = 0 ; i < S ; i ++) {
		las::gpstime out;
		decomp.decompressWith(decoder, (char *)&out);

		EXPECT_EQ(out.value, vs[i]);
	}
}

TEST(lazperf_tests, can_compress_decompress_rgb) {
	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::rgb>
	> comp;

	const size_t rs = 1, gs = 1, bs = 1;
	const size_t rl = 1 << 16, gl = 1 << 16, bl = 1 << 16;

	for (size_t r = rs ; r < rl ; r <<= 1) {
		for (size_t g = gs ; g < gl ; g <<= 1) {
			for (size_t b = bs ; b < bl ; b <<= 1) {
				las::rgb c((unsigned short)r, (unsigned short)g, (unsigned short)b);
				comp.compressWith(encoder, (char*)&c);
			}
		}
	}

	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::rgb>
	> decomp;

	for (size_t r = rs ; r < rl ; r <<= 1) {
		for (size_t g = gs ; g < gl ; g <<= 1) {
			for (size_t b = bs ; b < bl ; b <<= 1) {
				las::rgb c((unsigned short)r, (unsigned short)g, (unsigned short)b);

				las::rgb out;

				decomp.decompressWith(decoder, (char *)&out);

				EXPECT_EQ(out.r, c.r);
				EXPECT_EQ(out.g, c.g);
				EXPECT_EQ(out.b, c.b);
			}
		}
	}
}

TEST(lazperf_tests, can_compress_decompress_rgb_single_channel) {
	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::rgb>
	> comp;

	const int S = 10000;
	std::vector<unsigned short> cols(S);

	srand((unsigned int)time(NULL));
	int rvalue = rand();
	for (size_t i = 0 ; i < S ; i++) {
		unsigned short col = rvalue % (1 << 16);
		cols[i] = col;

		las::rgb c(col, col, col);
		comp.compressWith(encoder, (char*)&c);
	}
	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::rgb>
	> decomp;

	for (size_t i = 0 ; i < S ; i++) {
		unsigned short col = cols[i];

		las::rgb c(col, col, col);
		las::rgb out;

		decomp.decompressWith(decoder, (char *)&out);

		EXPECT_EQ(out.r, c.r);
		EXPECT_EQ(out.g, c.g);
		EXPECT_EQ(out.b, c.b);
	}
}

TEST(lazperf_tests, can_compress_decompress_real_gpstime) {
	reader las(testFile("point-time.las"));

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::gpstime>
	> comp;

//	std::cout << "file: " << las.size_ << ", " << las.count_ << std::endl;

	struct {
		las::point10 p;
		las::gpstime t;
	} p;

	unsigned int l = las.count_;
	std::vector<int64_t> ts;
	for (unsigned int i = 0 ; i < l ; i ++) {
		las.record((char*)&p);
		ts.push_back(p.t.value);
		comp.compressWith(encoder, (char*)&p.t.value);
	}
	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::gpstime>
	> decomp;

	for (size_t i = 0 ; i < l ; i ++) {
		las::gpstime t;
		decomp.decompressWith(decoder, (char*)&t);
		EXPECT_EQ(ts[i], t.value);
	}
}

TEST(lazperf_tests, can_compress_decompress_real_color) {
	reader las(testFile("point-color.las"));

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::rgb>
	> comp;

//	std::cout << "file: " << las.size_ << ", " << las.count_ << std::endl;

	struct {
		las::point10 p;
		las::rgb c;
	} p;

	unsigned int l = std::min(10u, las.count_);
	std::vector<las::rgb> ts;
	for (unsigned int i = 0 ; i < l ; i ++) {
		las.record((char*)&p);
		ts.push_back(p.c);
		comp.compressWith(encoder, (char*)&p.c);
	}
	encoder.done();

	decoders::arithmetic<SuchStream> decoder(s);
	record_decompressor<
		field<las::rgb>
	> decomp;

	for (size_t i = 0 ; i < l ; i ++) {
		las::rgb t;
		decomp.decompressWith(decoder, (char*)&t);
		EXPECT_EQ(ts[i].r, t.r);
		EXPECT_EQ(ts[i].g, t.g);
		EXPECT_EQ(ts[i].b, t.b);
	}
}

TEST(lazperf_tests, can_encode_match_laszip_point10time) {
	reader laz(testFile("point-time.las.laz")),
           las(testFile("point-time.las"));

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::point10>,
		field<las::gpstime>
	> comp;

	struct {
		las::point10 p;
		las::gpstime t;
	} p;

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

//	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < s.buf.size(); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, can_encode_match_laszip_point10color) {
	reader laz(testFile("point-color.las.laz")),
		   las(testFile("point-color.las"));

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::point10>,
		field<las::rgb>
	> comp;

#pragma pack(push, 1)
	struct {
		las::point10 p;
		las::rgb c;
	} p;
#pragma pack(pop)

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);
//		std::cout << "i = " << i << ", c: " << p.c.r << ", " << p.c.g << ", " << p.c.b << std::endl;
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

//	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < std::min(30u, (unsigned int)s.buf.size()); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, can_encode_match_laszip_point10timecolor) {
	reader laz(testFile("point-color-time.las.laz")),
		   las(testFile("point-color-time.las"));

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::point10>,
		field<las::gpstime>,
		field<las::rgb>
	> comp;

#pragma pack(push, 1)
	struct {
		las::point10 p;
		las::gpstime t;
		las::rgb c;
	} p;
#pragma pack(pop)

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);

//		std::cout << "i = " << i << ", c: " << p.c.r << ", " << p.c.g << ", " << p.c.b << std::endl;
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < std::min(30u, (unsigned int)s.buf.size()); i ++) {
		EXPECT_EQ(s.buf[i], laz.byte());
	}
}

TEST(lazperf_tests, schema_to_point_format_works) {
	using namespace laszip;
	using namespace laszip::factory;

	{
		record_schema s;
		s(record_item(record_item::POINT10));

		EXPECT_EQ(schema_to_point_format(s), 0);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::GPSTIME));

		EXPECT_EQ(schema_to_point_format(s), 1);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::RGB12));

		EXPECT_EQ(schema_to_point_format(s), 2);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::GPSTIME))
			(record_item(record_item::RGB12));

		EXPECT_EQ(schema_to_point_format(s), 3);
	}

	// Make sure we bail if something is not supported
	{
		auto f1 = []() {
			record_schema s;
			s(record_item(record_item::GPSTIME));

			schema_to_point_format(s);
		};

		auto f2 = []() {
			record_schema s;
			s(record_item(record_item::GPSTIME))
				(record_item(record_item::POINT10))
				(record_item(record_item::RGB12));

			schema_to_point_format(s);
		};

		auto f3 = []() {
			record_schema s;
			schema_to_point_format(s);
		};

		EXPECT_THROW(f1(), unknown_schema_type);
		EXPECT_THROW(f2(), unknown_schema_type);
		EXPECT_THROW(f3(), unknown_schema_type);
	}
}

TEST(lazperf_tests, just_xyz_encodes_and_decodes) {
    const int POINT_COUNT = 100000;

	using namespace laszip;
	using namespace laszip::formats;

    las::xyz input;

	record_compressor<
		field<las::xyz>
    > compressor;

	SuchStream s;

	encoders::arithmetic<SuchStream> encoder(s);

    unsigned int seed = static_cast<unsigned int>(time(NULL));
    srand(seed);
	int rvalue = rand();
	for (int i = 0 ; i < POINT_COUNT; i ++) {
        input.x = rvalue;
        input.y = rvalue;
        input.z = rvalue;

		compressor.compressWith(encoder, (const char*)&input);
	}
	encoder.done();

	record_decompressor<
		field<las::xyz>
    > decompressor;

	decoders::arithmetic<SuchStream> decoder(s);


    srand(seed);
	for (int i = 0 ; i < POINT_COUNT ; i ++) {
		decompressor.decompressWith(decoder, (char *)&input);

		EXPECT_EQ(input.x, rvalue);
		EXPECT_EQ(input.y, rvalue);
		EXPECT_EQ(input.z, rvalue);
	}
}

TEST(lazperf_tests, dynamic_field_compressor_works) {
    const int POINT_COUNT = 1000;

	using namespace laszip;
	using namespace laszip::formats;

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<int>();


		unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);
		int rvalue = rand();

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            int a = rvalue;
            comp->compress((const char*)&a);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<int>();

        srand(seed);
        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            int a = 0;
            decomp->decompress((char *)&a);

            EXPECT_EQ(a, rvalue);
        }
    }

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();

        int arr[10];


		unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);
		int rvalue = rand();

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            for (int j = 0 ; j < 10 ; j ++) {
                arr[j] = rvalue;
            }

            comp->compress((const char*)arr);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();

        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)arr);

            for (int j = 0 ; j < 10 ; j ++) {
                EXPECT_EQ(arr[j], rvalue);
            }
        }
    }

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<las::gpstime>();


		unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);
		int rvalue = rand();

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            las::gpstime g(makegps(rvalue, rvalue));
            comp->compress((const char*)&g);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<las::gpstime>();

        srand(seed);
        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            las::gpstime a;
            decomp->decompress((char *)&a);

            EXPECT_EQ(a.value, makegps(rvalue, rvalue));
        }
    }

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<las::gpstime>();
        comp->add_field<las::rgb>();
        comp->add_field<short>();
        comp->add_field<unsigned short>();
        comp->add_field<int>();

        // make sure if you're writing structs directly, they need to be packed in tight
#pragma pack(push, 1)
        struct {
            las::gpstime t;
            las::rgb c;
            int16_t a;
            uint16_t b;
            int32_t d;
        } data;
#pragma pack(pop)

		int rvalue = rand();
        auto randshort = [rvalue]() -> short {
            return rvalue % std::numeric_limits<short>::max();
        };

        auto randushort = [rvalue]() -> unsigned short {
            return rvalue % std::numeric_limits<unsigned short>::max();
        };

		unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);


        uint16_t r, g, b;
        int t1, t2;
        for (int i = 0 ; i < POINT_COUNT; i ++) {
            t1 = rvalue;
            t2 = rvalue;
            data.t = las::gpstime(makegps(t1, t2));
            r = randushort();
            g = randushort();
            b = randushort();
            data.c = las::rgb(r, g, b);
            data.a = randshort();
            data.b = randushort();
            data.d =  rvalue;

            comp->compress((const char*)&data);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<las::gpstime>();
        decomp->add_field<las::rgb>();
        decomp->add_field<short>();
        decomp->add_field<unsigned short>();
        decomp->add_field<int>();

        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)&data);
            int t3 = rvalue;
            int t4 = rvalue;
            EXPECT_EQ(data.t.value, makegps(t3, t4));
            EXPECT_EQ(data.c.r, randushort());
            EXPECT_EQ(data.c.g, randushort());
            EXPECT_EQ(data.c.b, randushort());
            EXPECT_EQ(data.a, randshort());
            EXPECT_EQ(data.b, randushort());
            EXPECT_EQ(data.d, rvalue);
        }
    }
}

TEST(lazperf_tests, dynamic_can_do_blind_compression) {
    const int POINT_COUNT = 10000;

	using namespace laszip;
	using namespace laszip::formats;

#pragma pack(push, 1)
    struct {
        double x, y, z;
        float  r, g, b;
    } p1, p2;
#pragma pack(pop)

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<las::gpstime>();
        comp->add_field<las::gpstime>();
        comp->add_field<las::gpstime>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();

        unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);
		int rvalue = rand();

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            p1.x = static_cast<double>(rvalue);
            p1.y = static_cast<double>(rvalue);
            p1.z = static_cast<double>(rvalue);

            p1.r = static_cast<float>(rvalue);
            p1.g = static_cast<float>(rvalue);
            p1.b = static_cast<float>(rvalue);

            comp->compress((const char*)&p1);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<las::gpstime>();
        decomp->add_field<las::gpstime>();
        decomp->add_field<las::gpstime>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();

        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)&p2);

            EXPECT_EQ(p2.x, static_cast<double>(rvalue));
            EXPECT_EQ(p2.y, static_cast<double>(rvalue));
            EXPECT_EQ(p2.z, static_cast<double>(rvalue));
            EXPECT_EQ(p2.r, static_cast<float>(rvalue));
            EXPECT_EQ(p2.g, static_cast<float>(rvalue));
            EXPECT_EQ(p2.b, static_cast<float>(rvalue));
        }
    }
    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<las::gpstime>();
        comp->add_field<las::gpstime>();
        comp->add_field<las::gpstime>();
        comp->add_field<int>();
        comp->add_field<int>();
        comp->add_field<int>();

		unsigned int seed = static_cast<unsigned int>(time(NULL));
        srand(seed);
		int rvalue = rand();

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            p1.x = static_cast<double>(rvalue) / static_cast<double>(rvalue);
            p1.y = static_cast<double>(rvalue) / static_cast<double>(rvalue);
            p1.z = static_cast<double>(rvalue) / static_cast<double>(rvalue);

            p1.r = static_cast<float>(static_cast<double>(rvalue) / static_cast<double>(rvalue));
            p1.g = static_cast<float>(static_cast<double>(rvalue) / static_cast<double>(rvalue));
            p1.b = static_cast<float>(static_cast<double>(rvalue) / static_cast<double>(rvalue));

            comp->compress((const char*)&p1);
        }
        encoder.done();

        decoders::arithmetic<SuchStream> decoder(s);
        auto decomp = make_dynamic_decompressor(decoder);

        decomp->add_field<las::gpstime>();
        decomp->add_field<las::gpstime>();
        decomp->add_field<las::gpstime>();
        decomp->add_field<int>();
        decomp->add_field<int>();
        decomp->add_field<int>();

        srand(seed);
        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)&p2);

            EXPECT_DOUBLE_EQ(p2.x, static_cast<double>(rvalue) / static_cast<double>(rvalue));
            EXPECT_DOUBLE_EQ(p2.y, static_cast<double>(rvalue) / static_cast<double>(rvalue));
            EXPECT_DOUBLE_EQ(p2.z, static_cast<double>(rvalue) / static_cast<double>(rvalue));
            EXPECT_FLOAT_EQ(p2.r, static_cast<float>(rvalue) / static_cast<float>(rvalue));
            EXPECT_FLOAT_EQ(p2.g, static_cast<float>(rvalue) / static_cast<float>(rvalue));
            EXPECT_FLOAT_EQ(p2.b, static_cast<float>(rvalue) / static_cast<float>(rvalue));
        }
    }
}
