/*
===============================================================================

  FILE:  brute_point10.cpp

  CONTENTS:
    Brute force benchmarking

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

#include "common/common.hpp"

#include "compressor.hpp"
#include "decompressor.hpp"

#include "encoder.hpp"
#include "decoder.hpp"
#include "formats.hpp"

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
	struct {
		int a;
		short b;
		unsigned short c;
		unsigned int d;
	} data;

	printf("%15s %15s %15s %15s %15s %15s\n",
			"Count",
			"Comp Init",
			"Comp Time",
			"Comp Flush",
			"Decomp Init",
			"Decomp Time");

	for (int N = 1000 ; N <= 1000000 ; N *= 10) {
		float
			compressorInitTime = 0.0,
			compressTime = 0.0,
			comrpressFlushTime = 0.0;

		float
			decompInitTime = 0.0,
			decompTime = 0.0;

		SuchStream s;

		auto tp = common::tick();

		record_compressor<
			field<int>,
			field<short>,
			field<unsigned short>,
			field<unsigned int> > compressor;

		encoders::arithmetic<SuchStream> encoder(s);

		compressorInitTime = common::since(tp);


		for (int i = 0 ; i < N; i ++) {
			data.a = i;
			data.b = i % (1 << 15) ;
			data.c = i % (1 << 16);
			data.d = i + (1 << 31);

			tp = common::tick();
			compressor.compressWith(encoder, (const char*)&data);
			compressTime += common::since(tp);
		}

		compressTime -= s.totalTime; // take out all the stream handling time
		s.totalTime = 0.0f;

		tp = common::tick();
		encoder.done();
		comrpressFlushTime = common::since(tp) - s.totalTime;

		s.totalTime = 0.0f;

		tp = common::tick();

		record_decompressor<
			field<int>,
			field<short>,
			field<unsigned short>,
			field<unsigned int> > decompressor;

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
			decompressor.decompressWith(decoder, (char *)&data);
			decompTime += common::since(tp);

			// Finally make sure things match, otherwise bail
			if (data.a != i ||
				data.b != i % (1 << 15) ||
				data.c != i % (1 << 16) ||
				data.d != (unsigned int)i + (1 << 31))
				throw std::runtime_error("Failure!");
		}

		decompTime -= s.totalTime;
		s.totalTime = 0.0f;


		// print results from this round
		printf("%15d %15.6f %15.6f %15.6f %15.6f %15.6f\n",
				N,
				compressorInitTime, compressTime, comrpressFlushTime,
				decompInitTime, decompTime);
	}


	printf("\n");
	printf("\tAll times in seconds.\n");
	printf("\tComp = Compressor\n\tDecomp = Decompressor\n\n");

	return 0;
}
