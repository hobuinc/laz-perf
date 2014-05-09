// proj_test.cpp
// proj backend tests
//
#include <boost/test/unit_test.hpp>


#include <fstream>

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"
#include "las.hpp"

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
	BOOST_CHECK_EQUAL(0xeadbeef, v);

	packers<unsigned short>::pack(0xbeef, buf);
	v = packers<unsigned short>::unpack(buf);
	BOOST_CHECK_EQUAL(0xbeef, v);

	packers<short>::pack(0xeef, buf);
	v = packers<short>::unpack(buf);
	BOOST_CHECK_EQUAL(0xeef, v);

	packers<unsigned char>::pack(0xf, buf);
	v = packers<unsigned char>::unpack(buf);
	BOOST_CHECK_EQUAL(0xf, v);

	packers<char>::pack(0x7, buf);
	v = packers<char>::unpack(buf);
	BOOST_CHECK_EQUAL(0x7, v);
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
		data.b = i + 10;
		data.c = i + 40000;
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
		data.b = i;
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

		p.intensity = i + (1<14);
		p.return_number =  i & 0x7;
		p.number_of_returns_of_given_pulse = (i + 4) & 0x7;
		p.scan_angle_rank = i & 1;
		p.edge_of_flight_line = (i+1) & 1;
		p.classification = (i + (1 << 7)) % (1 << 8);
		p.scan_angle_rank = i % (1 << 7);
		p.user_data = (i + 64) % (1 << 7);
		p.point_source_ID = i;

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
		/*
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
		*/
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
	size_t count = 0;

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
	size_t fileSize = f.tellg();
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
	size_t count = fin.tellg() / sizeof(las::point10);
	fin.seekg(0);

	size_t index = 0;
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
	size_t count = f.tellg() / sizeof(las::point10);
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
	size_t count = f.tellg() / sizeof(las::point10);
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
	size_t fileSize = f.tellg();
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
	size_t count = fin.tellg() / sizeof(las::point10);
	fin.seekg(0);

	size_t index = 0;
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

	srand(time(NULL));
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
				las::rgb c(r, g, b);
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
				las::rgb c(r, g, b);

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

	srand(time(NULL));
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


// A simple classes that offsets to the data for us
struct reader {
	reader(const std::string& f) :
		f_(f, std::ios::binary) {
#define good_or_bail(msg) if(!f_.good()) { f_.close(); throw std::runtime_error(f + std::string(" : ") + msg);}

			good_or_bail("Could not open file");

			// read data offset and jump to it
			f_.seekg(32*3); good_or_bail("Could not read data offset");

			unsigned int data = 0;
			f_.read((char*)&data, 4); good_or_bail("Could not read data offset");

			size_ = 0;
			f_.seekg(32*3+8+1); good_or_bail("Could not read record size");
			f_.read((char *)&size_, sizeof(size_)); good_or_bail("Could not read record size");
			f_.read((char *)&count_, sizeof(count_)); good_or_bail("Could not read points count");

			f_.seekg(data); good_or_bail("Could not offset to data offset");
		}

	~reader() {
		f_.close();
	}


	void skip(size_t n) {
		f_.seekg(n, std::ios::cur);
	}

	unsigned char byte() {
		return static_cast<unsigned char>(f_.get());
	}

	void record(char *b) { // make sure you pass enough memory
		f_.read(b, size_);
	}

	std::ifstream f_;
	unsigned short size_;
	unsigned int count_;
};

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
};

/*
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

	std::cout << "file: " << las.size_ << ", " << las.count_ << std::endl;

	struct {
		las::point10 p;
		las::gpstime t;
	} p;

	for (unsigned int i = 0 ; i < las.count_ ; i ++) {
		las.record((char*)&p);
		std::cout << "i = " << i << ", " << p.t.value << std::endl;
		comp.compressWith(encoder, (char*)&p);
	}
	encoder.done();

	std::cout << "buffer size: " << s.buf.size() << std::endl;

	laz.skip(8); // jump past the chunk table offset
	for (size_t i = 0 ; i < std::min(1015ul, s.buf.size()); i ++) {
		BOOST_CHECK_EQUAL(s.buf[i], laz.byte());
	}
}
*/

BOOST_AUTO_TEST_SUITE_END()
