/*
===============================================================================

  FILE:  tlaz_tests.cpp

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


#include <boost/test/unit_test.hpp>
#include <ctime>


#include <fstream>

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"
#include "las.hpp"
#include "factory.hpp"

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

BOOST_AUTO_TEST_SUITE(tlaz_tests)


BOOST_AUTO_TEST_CASE(packers_are_symmetric) {
	using namespace laszip::formats;

	char buf[4];
	packers<unsigned int>::pack(0xdeadbeaf, buf);
	unsigned int v = packers<unsigned int>::unpack(buf);
	BOOST_CHECK_EQUAL(v, 0xdeadbeaf);

	packers<int>::pack(0xeadbeef, buf);
	v = packers<int>::unpack(buf);
	BOOST_CHECK_EQUAL(0xeadbeefu, v);

	packers<unsigned short>::pack(0xbeef, buf);
	v = packers<unsigned short>::unpack(buf);
	BOOST_CHECK_EQUAL(0xbeefu, v);

	packers<short>::pack(0xeef, buf);
	v = packers<short>::unpack(buf);
	BOOST_CHECK_EQUAL(0xeefu, v);

	packers<unsigned char>::pack(0xf, buf);
	v = packers<unsigned char>::unpack(buf);
	BOOST_CHECK_EQUAL(0xfu, v);

	packers<char>::pack(0x7, buf);
	v = packers<char>::unpack(buf);
	BOOST_CHECK_EQUAL(0x7u, v);
}


BOOST_AUTO_TEST_CASE(packers_canpack_gpstime) {
	using namespace laszip::formats;

	{
		las::gpstime v(std::numeric_limits<int64_t>::max());
		char buf[8];

		packers<las::gpstime>::pack(v, buf);
		las::gpstime out = packers<las::gpstime>::unpack(buf);

		BOOST_CHECK_EQUAL(v.value, out.value);
		BOOST_CHECK_EQUAL(std::equal(buf, buf + 8, (char *)&v.value), true);
	}

	{
		las::gpstime v(std::numeric_limits<int64_t>::min());
		char buf[8];

		packers<las::gpstime>::pack(v, buf);
		las::gpstime out = packers<las::gpstime>::unpack(buf);

		BOOST_CHECK_EQUAL(v.value, out.value);
		BOOST_CHECK_EQUAL(std::equal(buf, buf + 8, (char *)&v.value), true);
	}
}

BOOST_AUTO_TEST_CASE(packers_canpack_rgb) {
	using namespace laszip::formats;

	las::rgb c(1<<15, 1<<14, 1<<13);
	char buf[6];

	packers<las::rgb>::pack(c, buf);
	las::rgb out = packers<las::rgb>::unpack(buf);

	BOOST_CHECK_EQUAL(c.r, out.r);
	BOOST_CHECK_EQUAL(c.g, out.g);
	BOOST_CHECK_EQUAL(c.b, out.b);

	BOOST_CHECK_EQUAL(std::equal(buf, buf+2, (char*)&c.r), true);
	BOOST_CHECK_EQUAL(std::equal(buf+2, buf+4, (char*)&c.g), true);
	BOOST_CHECK_EQUAL(std::equal(buf+4, buf+6, (char*)&c.b), true);
}

BOOST_AUTO_TEST_CASE(las_structs_are_of_correct_size) {
	using namespace laszip::formats;

	BOOST_CHECK_EQUAL(sizeof(las::point10), 20u);
	BOOST_CHECK_EQUAL(sizeof(las::gpstime), 8u);
	BOOST_CHECK_EQUAL(sizeof(las::rgb), 6u);
}

BOOST_AUTO_TEST_CASE(works_with_fields) {
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

		BOOST_CHECK_EQUAL(data.a, i);
		BOOST_CHECK_EQUAL(data.b, i+10);
		BOOST_CHECK_EQUAL(data.c, i+40000);
		BOOST_CHECK_EQUAL(data.d, (unsigned int)i+ (1<<31));
	}
}

BOOST_AUTO_TEST_CASE(works_with_one_field) {
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
		BOOST_CHECK_EQUAL(data.a, i);
	}
}

BOOST_AUTO_TEST_CASE(works_with_all_kinds_of_fields) {
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
		BOOST_CHECK_EQUAL(data.a, i);
		BOOST_CHECK_EQUAL(data.ua, (unsigned int)i + (1<<31));
		BOOST_CHECK_EQUAL(data.b, i);
		BOOST_CHECK_EQUAL(data.ub, (unsigned short)i + (1<<15));
		BOOST_CHECK_EQUAL(data.c, i % 128);
		BOOST_CHECK_EQUAL(data.uc, (unsigned char)(i % 128) + (1<<7));
	}
}


BOOST_AUTO_TEST_CASE(correctly_packs_unpacks_point10) {
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
		BOOST_CHECK_EQUAL(pout.x, p.x);
		BOOST_CHECK_EQUAL(pout.y, p.y);
		BOOST_CHECK_EQUAL(pout.z, p.z);

		BOOST_CHECK_EQUAL(pout.intensity, p.intensity);
		BOOST_CHECK_EQUAL(pout.return_number, p.return_number);
		BOOST_CHECK_EQUAL(pout.number_of_returns_of_given_pulse, p.number_of_returns_of_given_pulse);
		BOOST_CHECK_EQUAL(pout.scan_angle_rank, p.scan_angle_rank);
		BOOST_CHECK_EQUAL(pout.edge_of_flight_line, p.edge_of_flight_line);
		BOOST_CHECK_EQUAL(pout.classification, p.classification);
		BOOST_CHECK_EQUAL(pout.scan_angle_rank, p.scan_angle_rank);
		BOOST_CHECK_EQUAL(pout.user_data, p.user_data);
		BOOST_CHECK_EQUAL(pout.point_source_ID, p.point_source_ID);
	}
}

BOOST_AUTO_TEST_CASE(point10_enc_dec_is_sym) {
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

		BOOST_CHECK_EQUAL(p.x, i);
		BOOST_CHECK_EQUAL(p.y, i + 1000);
		BOOST_CHECK_EQUAL(p.z, i + 10000);

		BOOST_CHECK_EQUAL(p.intensity, (i + (1<14)) % (1 << 16));
		BOOST_CHECK_EQUAL(p.return_number,  i & 0x7);
		BOOST_CHECK_EQUAL(p.number_of_returns_of_given_pulse, (i + 4) & 0x7);
		BOOST_CHECK_EQUAL(p.scan_direction_flag, i & 1);
		BOOST_CHECK_EQUAL(p.edge_of_flight_line, (i+1) & 1);
		BOOST_CHECK_EQUAL(p.classification, (i + (1 << 7)) % (1 << 8));
		BOOST_CHECK_EQUAL(p.scan_angle_rank, i % (1 << 7));
		BOOST_CHECK_EQUAL(p.user_data, (i + 64) % (1 << 7));
		BOOST_CHECK_EQUAL(p.point_source_ID, i % (1 << 16));
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

BOOST_AUTO_TEST_CASE(can_compress_decompress_real_data) {
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f("test/raw-sets/point10-1.las.raw", std::ios::binary);
	if (!f.good())
		BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

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


BOOST_AUTO_TEST_CASE(can_decode_laszip_buffer) {
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f("test/raw-sets/point10-1.las.laz.raw", std::ios::binary);
	if (!f.good())
		BOOST_FAIL("Raw LAZ file not available. Make sure you're running tests from the root of the project.");

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
	std::ifstream fin("test/raw-sets/point10-1.las.raw", std::ios::binary);
	if (!fin.good())
		BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

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

	fin.close();
}

void matchSets(const std::string& lasRaw, const std::string& lazRaw) {
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f(lasRaw, std::ios::binary);
	if (!f.good())
		BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

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
		BOOST_FAIL("Raw LAZ file not available. Make sure you're running tests from the root of the project.");

	// Make sure things match
	for (size_t i = 0 ; i < s.buf.size() && !fc.eof() ; i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], fc.get());
	}

	fc.close();
}

BOOST_AUTO_TEST_CASE(binary_matches_laszip) {
	matchSets("test/raw-sets/point10-1.las.raw",
			"test/raw-sets/point10-1.las.laz.raw");
}


BOOST_AUTO_TEST_CASE(dynamic_compressor_works) {
	using namespace laszip;
	using namespace laszip::formats;

	const std::string lasRaw = "test/raw-sets/point10-1.las.raw";
	const std::string lazRaw = "test/raw-sets/point10-1.las.laz.raw";

	typedef encoders::arithmetic<SuchStream> Encoder;

	SuchStream s;
	Encoder encoder(s);

	auto compressor = new record_compressor<field<las::point10> >();

	dynamic_compressor::ptr pcompressor =
		make_dynamic_compressor(encoder, compressor);

	std::ifstream f(lasRaw, std::ios::binary);
	if (!f.good())
		BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

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
		BOOST_FAIL("Raw LAZ file not available. Make sure you're running tests from the root of the project.");

	// Make sure things match
	for (size_t i = 0 ; i < s.buf.size() && !fc.eof() ; i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], fc.get());
	}

	fc.close();
}


BOOST_AUTO_TEST_CASE(dynamic_decompressor_can_decode_laszip_buffer) {
	using namespace laszip;
	using namespace laszip::formats;

	std::ifstream f("test/raw-sets/point10-1.las.laz.raw", std::ios::binary);
	if (!f.good())
		BOOST_FAIL("Raw LAZ file not available. Make sure you're running tests from the root of the project.");

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
	std::ifstream fin("test/raw-sets/point10-1.las.raw", std::ios::binary);
	if (!fin.good())
		BOOST_FAIL("Raw LAS file not available. Make sure you're running tests from the root of the project.");

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

	fin.close();
}

int64_t makegps(unsigned int upper, unsigned int lower) {
	int64_t u = upper,
			l = lower;
	return (u << 32) | l;
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_gpstime) {
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
			las::gpstime t(makegps((i > 0 ? 1ll << i : 0), (j << 16) + j));
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
			las::gpstime t(makegps((i > 0 ? 1ll << i : 0), (j << 16) + j));
			las::gpstime out;
			decomp.decompressWith(decoder, (char *)&out);

			BOOST_CHECK_EQUAL(out.value, t.value);
		}
	}
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_random_gpstime) {
	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::gpstime>
	> comp;

	const size_t S = 1000;

	srand((unsigned int)std::time(NULL));
	std::vector<int64_t> vs(S);
	for (size_t i = 0 ; i < S ; i ++) {
		int64_t a = rand() & 0xFFFF,
				b = rand() & 0xFFFF,
				c = rand() & 0xFFFF,
				d = rand() & 0xFFFF;

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

		BOOST_CHECK_EQUAL(out.value, vs[i]);
	}
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_rgb) {
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

				BOOST_CHECK_EQUAL(out.r, c.r);
				BOOST_CHECK_EQUAL(out.g, c.g);
				BOOST_CHECK_EQUAL(out.b, c.b);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_rgb_single_channel) {
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
	for (size_t i = 0 ; i < S ; i++) {
		unsigned short col = rand() % (1 << 16);
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

		BOOST_CHECK_EQUAL(out.r, c.r);
		BOOST_CHECK_EQUAL(out.g, c.g);
		BOOST_CHECK_EQUAL(out.b, c.b);
	}
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_real_gpstime) {
	reader las("test/raw-sets/point-time.las");

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::gpstime>
	> comp;

	std::cout << "file: " << las.size_ << ", " << las.count_ << std::endl;

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
		BOOST_CHECK_EQUAL(ts[i], t.value);
	}
}

BOOST_AUTO_TEST_CASE(can_compress_decompress_real_color) {
	reader las("test/raw-sets/point-color.las");

	using namespace laszip;
	using namespace laszip::formats;

	SuchStream s;
	encoders::arithmetic<SuchStream> encoder(s);

	record_compressor<
		field<las::rgb>
	> comp;

	std::cout << "file: " << las.size_ << ", " << las.count_ << std::endl;

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
		BOOST_CHECK_EQUAL(ts[i].r, t.r);
		BOOST_CHECK_EQUAL(ts[i].g, t.g);
		BOOST_CHECK_EQUAL(ts[i].b, t.b);
	}
}

BOOST_AUTO_TEST_CASE(can_encode_match_laszip_point10time) {
	reader laz("test/raw-sets/point-time.las.laz"),
		   las("test/raw-sets/point-time.las");

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

	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < s.buf.size(); i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], laz.byte());
	}
}

BOOST_AUTO_TEST_CASE(can_encode_match_laszip_point10color) {
	reader laz("test/raw-sets/point-color.las.laz"),
		   las("test/raw-sets/point-color.las");

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
		std::cout << "i = " << i << ", c: " << p.c.r << ", " << p.c.g << ", " << p.c.b << std::endl;
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < std::min(30u, (unsigned int)s.buf.size()); i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], laz.byte());
	}
}

BOOST_AUTO_TEST_CASE(can_encode_match_laszip_point10timecolor) {
	reader laz("test/raw-sets/point-color-time.las.laz"),
		   las("test/raw-sets/point-color-time.las");

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

		std::cout << "i = " << i << ", c: " << p.c.r << ", " << p.c.g << ", " << p.c.b << std::endl;
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < std::min(30u, (unsigned int)s.buf.size()); i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], laz.byte());
	}
}

BOOST_AUTO_TEST_CASE(schema_to_point_format_works) {
	using namespace laszip;
	using namespace laszip::factory;

	{
		record_schema s;
		s(record_item(record_item::POINT10));

		BOOST_CHECK_EQUAL(schema_to_point_format(s), 0);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::GPSTIME));

		BOOST_CHECK_EQUAL(schema_to_point_format(s), 1);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::RGB12));

		BOOST_CHECK_EQUAL(schema_to_point_format(s), 2);
	}

	{
		record_schema s;
		s(record_item(record_item::POINT10))
			(record_item(record_item::GPSTIME))
			(record_item(record_item::RGB12));

		BOOST_CHECK_EQUAL(schema_to_point_format(s), 3);
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

		BOOST_CHECK_THROW(f1(), unknown_schema_type);
		BOOST_CHECK_THROW(f2(), unknown_schema_type);
		BOOST_CHECK_THROW(f3(), unknown_schema_type);
	}
}

BOOST_AUTO_TEST_CASE(just_xyz_encodes_and_decodes) {
    const int POINT_COUNT = 100000;

	using namespace laszip;
	using namespace laszip::formats;

    las::xyz input;

	record_compressor<
		field<las::xyz>
    > compressor;

	SuchStream s;

	encoders::arithmetic<SuchStream> encoder(s);

    time_t seed = time(NULL);

    srand(seed);
	for (int i = 0 ; i < POINT_COUNT; i ++) {
        input.x = rand();
        input.y = rand();
        input.z = rand();

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

		BOOST_CHECK_EQUAL(input.x, rand());
		BOOST_CHECK_EQUAL(input.y, rand());
		BOOST_CHECK_EQUAL(input.z, rand());
	}
}

BOOST_AUTO_TEST_CASE(dynamic_field_compressor_works) {
    const int POINT_COUNT = 1000;

	using namespace laszip;
	using namespace laszip::formats;

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<int>();


        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            int a = rand();
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

            BOOST_CHECK_EQUAL(a, rand());
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


        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            for (int j = 0 ; j < 10 ; j ++) {
                arr[j] = rand();
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

        srand(seed);
        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)arr);

            for (int j = 0 ; j < 10 ; j ++) {
                BOOST_CHECK_EQUAL(arr[j], rand());
            }
        }
    }

    {
        SuchStream s;
        encoders::arithmetic<SuchStream> encoder(s);
        auto comp = make_dynamic_compressor(encoder);

        comp->add_field<las::gpstime>();


        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            las::gpstime g(makegps(rand(), rand()));
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

            BOOST_CHECK_EQUAL(a.value, makegps(rand(), rand()));
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
            short a;
            unsigned short b;
            int d;
        } data;
#pragma pack(pop)

        auto randshort = []() -> short {
            return rand() % std::numeric_limits<short>::max();
        };

        auto randushort = []() -> unsigned short {
            return rand() % std::numeric_limits<unsigned short>::max();
        };

        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            data.t = las::gpstime(makegps(rand(), rand()));
            data.c = las::rgb(randushort(), randushort(), randushort());
            data.a = randshort();
            data.b = randushort();
            data.d =  rand();

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

        srand(seed);
        for (int i = 0 ; i < POINT_COUNT ; i ++) {
            decomp->decompress((char *)&data);

            BOOST_CHECK_EQUAL(data.t.value, makegps(rand(), rand()));
            BOOST_CHECK_EQUAL(data.c.r, randushort());
            BOOST_CHECK_EQUAL(data.c.g, randushort());
            BOOST_CHECK_EQUAL(data.c.b, randushort());
            BOOST_CHECK_EQUAL(data.a, randshort());
            BOOST_CHECK_EQUAL(data.b, randushort());
            BOOST_CHECK_EQUAL(data.d, rand());
        }
    }
}

BOOST_AUTO_TEST_CASE(dynamic_can_do_blind_compression) {
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

        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            p1.x = static_cast<double>(rand());
            p1.y = static_cast<double>(rand());
            p1.z = static_cast<double>(rand());

            p1.r = static_cast<float>(rand());
            p1.g = static_cast<float>(rand());
            p1.b = static_cast<float>(rand());

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

            BOOST_CHECK_EQUAL(p2.x, static_cast<double>(rand()));
            BOOST_CHECK_EQUAL(p2.y, static_cast<double>(rand()));
            BOOST_CHECK_EQUAL(p2.z, static_cast<double>(rand()));
            BOOST_CHECK_EQUAL(p2.r, static_cast<float>(rand()));
            BOOST_CHECK_EQUAL(p2.g, static_cast<float>(rand()));
            BOOST_CHECK_EQUAL(p2.b, static_cast<float>(rand()));
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

        time_t seed = time(NULL);
        srand(seed);

        for (int i = 0 ; i < POINT_COUNT; i ++) {
            p1.x = static_cast<double>(rand()) / static_cast<double>(rand());
            p1.y = static_cast<double>(rand()) / static_cast<double>(rand());
            p1.z = static_cast<double>(rand()) / static_cast<double>(rand());

            p1.r = static_cast<float>(rand()) / static_cast<double>(rand());
            p1.g = static_cast<float>(rand()) / static_cast<double>(rand());
            p1.b = static_cast<float>(rand()) / static_cast<double>(rand());

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

            BOOST_CHECK_CLOSE(p2.x, static_cast<double>(rand()) / static_cast<double>(rand()), 0.000000000001);
            BOOST_CHECK_CLOSE(p2.y, static_cast<double>(rand()) / static_cast<double>(rand()), 0.000000000001);
            BOOST_CHECK_CLOSE(p2.z, static_cast<double>(rand()) / static_cast<double>(rand()), 0.000000000001);
            BOOST_CHECK_CLOSE(p2.r, static_cast<float>(rand()) / static_cast<double>(rand()), 0.00001);
            BOOST_CHECK_CLOSE(p2.g, static_cast<float>(rand()) / static_cast<double>(rand()), 0.00001);
            BOOST_CHECK_CLOSE(p2.b, static_cast<float>(rand()) / static_cast<double>(rand()), 0.00001);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
