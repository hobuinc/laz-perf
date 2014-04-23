// proj_test.cpp
// proj backend tests
//
#include <boost/test/unit_test.hpp>

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"

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

BOOST_AUTO_TEST_SUITE_END()
