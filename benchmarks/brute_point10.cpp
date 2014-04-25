// brute.cpp
// Brute force benchmarking
//

#include "../common/common.hpp"

#include "compressor.hpp"
#include "decompressor.hpp"

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"
#include "las.hpp"

#include <iostream>
#include <memory>
#include <stdio.h>

// We need to time operations from within the stream objects itself, may be fast for C++ but we need to know
// their overhead for js
//
struct SuchStream {
	SuchStream() : buf(), idx(0), totalTime(0.0f) {}

	void putBytes(const unsigned char* b, size_t len) {
		auto tp = common::tick();

		while(len --) {
			buf.push_back(*b++);
		}

		totalTime += common::since(tp);
	}

	void putByte(const unsigned char b) {
		auto tp = common::tick();
		buf.push_back(b);
		totalTime += common::since(tp);
	}

	unsigned char getByte() {
		auto tp = common::tick();
		unsigned char b = buf[idx++];
		totalTime += common::since(tp);

		return b;
	}

	void getBytes(unsigned char *b, int len) {
		auto tp = common::tick();
		for (int i = 0 ; i < len ; i ++) {
			b[i] = getByte();
		}
		totalTime += common::since(tp);
	}

	std::vector<unsigned char> buf;	// cuz I'm ze faste
	size_t idx;
	
	float totalTime;
};

int main() {
	// import namespaces to reduce typing
	//
	using namespace laszip;
	using namespace laszip::formats;

	// Let's say our record looks something like this:
	//
	printf("%15s %15s %15s %15s %15s %15s %15s %15s %15s\n",
			"Count",
			"Comp Init",
			"Comp Time",
			"Comp Flush",
			"Decomp Init",
			"Decomp Time",
			"Comp OH",
			"Decomp OH",
			"Total Time");

	for (int N = 1000 ; N <= 10000000 ; N *= 10) {
		auto totalTP = common::tick();

		float
			compressorInitTime = 0.0,
			compressTime = 0.0,
			comrpressFlushTime = 0.0;

		float
			decompInitTime = 0.0,
			decompTime = 0.0;

		float encoderStreamOverhead = 0.0,
			  decoderStreamOverhead = 0.0;

		SuchStream s;

		auto tp = common::tick();

		record_compressor<
			field<las::point10>
		> compressor;

		encoders::arithmetic<SuchStream> encoder(s);

		compressorInitTime = common::since(tp);

		las::point10 p;

		for (int i = 0 ; i < N; i ++) {
			p.x = i;
			p.y = i % (1 << 15) ;
			p.z = i % (1 << 16);
			p.intensity = i % (1 << 15);

			tp = common::tick();
			compressor.compressWith(encoder, (const char*)&p);
			compressTime += common::since(tp);
		}

		encoderStreamOverhead = s.totalTime;
		compressTime -= s.totalTime; // take out all the stream handling time
		s.totalTime = 0.0f;

		tp = common::tick();
		encoder.done();
		comrpressFlushTime = common::since(tp) - s.totalTime;

		s.totalTime = 0.0f;

		tp = common::tick();

		record_decompressor<
			field<las::point10>
		> decompressor;

		// Create a decoder same way as we did with the encoder
		//
		decoders::arithmetic<SuchStream> decoder(s);

		decompInitTime = common::since(tp);

		// This time we'd read the values out instead and make sure they match what we pushed in
		//
		for (int i = 0 ; i < N ; i ++) {
			// When we decompress data we need to provide where to decode stuff to
			//
			tp = common::tick();
			decompressor.decompressWith(decoder, (char *)&p);
			decompTime += common::since(tp);

			// Finally make sure things match, otherwise bail
			if (p.x != i ||
				p.y != i % (1 << 15) ||
				p.z != i % (1 << 16) ||
				p.intensity != i % (1 << 15))
				throw std::runtime_error("Failure!");
		}

		decoderStreamOverhead = s.totalTime;
		decompTime -= s.totalTime;
		s.totalTime = 0.0f;

		float totalRunTime = common::since(totalTP);


		// print results from this round
		printf("%15d %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f %15.6f\n",
				N,
				compressorInitTime, compressTime, comrpressFlushTime,
				decompInitTime, decompTime,
				encoderStreamOverhead, decoderStreamOverhead,
				totalRunTime);
	}


	printf("\n");
	printf("\tAll times in seconds.\n");
	printf("\tComp = Compressor\n\tDecomp = Decompressor\n\n");

	return 0;
}
