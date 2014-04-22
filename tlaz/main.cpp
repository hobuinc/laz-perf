// tlaz.cpp
// Run the templatize laz
//

#include "../common/common.hpp"
#include "compressor.hpp"
#include "decompressor.hpp"

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"

#include <iostream>
#include <memory>

template<
	typename TByte
>
struct SuchStream {
	SuchStream() : buf() {}

	void putBytes(const TByte* b, size_t len) {
		static_assert(std::is_same<TByte, unsigned char>::value,
				"For now try to pass in unsigned char data");

		while(len --) {
			buf.push_back(*b++);
		}
	}

	void putByte(const TByte& b) {
		buf.push_back(b);
	}

	std::vector<TByte> buf;	// cuz I'm ze faste
};

template<
	typename TByte
>
struct WowStream {
	template<typename TIn>
	WowStream(const TIn& buf) : offset_(0) {
		buf_.resize(buf.size());
		std::copy(buf.begin(), buf.end(), buf_.begin());
	}

	TByte getByte() {
		return buf_[offset_++];
	}

	std::vector<TByte> buf_;
	size_t offset_;
};

void runEncoderDecoder() {
	using namespace laszip;

	auto start = common::tick();
	std::cout << common::since(start) << std::endl;

	typedef SuchStream<U8> MuchInput;
	typedef WowStream<U8> MuchOutput;

	typedef encoders::arithmetic<MuchInput> Encoder;
	typedef compressors::integer<Encoder> Compressor;

	// Throw stuff down at the coder and capture the output
	MuchInput s;

	Encoder encoder(s);
	Compressor compressor(encoder, 32);

	compressor.init();

	int n = 1000000;
	int step = 10;

	std::cout << "Compressing " << n / step << " integers." << std::endl;
	for (int i = step ; i < n ; i += step) {
		compressor.compress(i - step, i, 0);
	}
	encoder.done();

	std::cout << "Compressed to: " << s.buf.size() << " bytes." << std::endl;
	std::cout << "Compressed to: " << 100 * (float)s.buf.size() / (sizeof(int) * (n / step)) << "%" << std::endl;

	std::cout << std::endl;
	std::cout << "Running decoder..." << std::endl;


	// Get stuff back from the encoded data
	MuchOutput o(s.buf);

	typedef decoders::arithmetic<MuchOutput> Decoder;
	typedef decompressors::integer<Decoder> Decompressor;

	Decoder decoder(o);
	Decompressor decompressor(decoder, 32);

	decompressor.init();

	for (int i = step ; i < n ; i += step) {
		auto v = decompressor.decompress(i - step, 0);
		if (v != i)
			throw std::runtime_error("Stoofs gown baed maen");
	}

	std::cout << "Decoded output matches!" << std::endl;
}

void formatStuff() {
	using namespace laszip::formats;
	using namespace laszip::compressors;
	using namespace laszip::encoders;

	typedef arithmetic<SuchStream<char> > EncoderType;

	SuchStream<char> s;

	EncoderType etype(s);

	record<
		EncoderType,
		field<int>,
		field<int>,
		field<short>,
		field<short> > r(etype);

	r.encode(NULL);
}

int main() {
	//runEncoderDecoder();
	formatStuff();
}
