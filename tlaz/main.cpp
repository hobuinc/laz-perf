// tlaz.cpp
// Run the templatize laz
//

#include "../common/common.hpp"
#include "compressor.hpp"
#include "encoder.hpp"

#include <iostream>
#include <memory>

struct SuchStream {
	SuchStream() : length(0) {}

	template<
		typename TByte
	>
	void putBytes(const TByte&, size_t len) {
		length += len;
	}

	template<
		typename TByte
	>
	void putByte(const TByte&) {
		length ++;
	}

	size_t length;
};

int main() {
	using namespace laszip;

	auto start = common::tick();
	std::cout << common::since(start) << std::endl;

	typedef encoders::arithmetic<SuchStream> Encoder;
	typedef compressors::integer<Encoder> Compressor;


	SuchStream s;

	Encoder encoder(s);
	Compressor compressor(encoder, 32);

	compressor.init();

	int n = 1000000;

	std::cout << "Compressing " << n << " integers." << std::endl;
	for (int i = 10 ; i < n ; i += 10) {
		compressor.compress(i - 10, i, 0); //, 19);
	}

	encoder.done();

	std::cout << "Compressed to: " << s.length << " bytes." << std::endl;
	std::cout << "Compressed to: " << 100 * (float)s.length / (sizeof(int) * n) << "%" << std::endl;

	return 0;
}
