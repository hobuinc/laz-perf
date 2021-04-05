// point10.cpp
// Test point10 stuff
//

#include "compressor.hpp"
#include "decompressor.hpp"

#include "las.hpp"
#include "encoder.hpp"
#include "decoder.hpp"

#include <iostream>
#include <memory>

lazperf::las::point10 p; // free init to zero :)

int main() {
	// import namespaces to reduce typing
	//
	using namespace lazperf;

	int N = 1000;

	// Instantiate the arithmetic encoder
	//
    MemoryStream s;
    point_compressor_0 compressor(s.outCb(), 0);

	// Encode some dummy data
	//
	for (int i = 0 ; i < N; i ++) {
		p.x = i;
		p.y = i + 1000;
		p.z = i + 10000;

		p.intensity = (unsigned short)(i + (1 << 15));
		p.return_number = (i >> 3) & 0x7;
		p.number_of_returns_of_given_pulse = i & 0x7;
		p.scan_direction_flag = i & 1;
		p.edge_of_flight_line = (i+1) & 1;
		p.classification = (unsigned char) i % 256;
		p.scan_angle_rank =  (unsigned char) i % 128;
		p.user_data = (i >> 4) % 256;
		p.point_source_ID = (i * 30) % (1 << 16);

		// All compressor cares about is your data as a pointer, it will unpack data
		// automatically based on the fields that were specified and compress them
		//
		compressor.compress((const char*)&p);
	}

	// Finally terminate the encoder by calling done on it. This will flush out any pending data to output.
	//
	compressor.done();

	// Print some fun stuff about compression
	//
	std::cout << "Points compressed to: " << s.buf.size() << " bytes" << std::endl;

	// Setup record decompressor for point10
	//
	// Create a decoder same way as we did with the encoder
	//
    point_decompressor_0 decompressor(s.inCb(), 0);

	// This time we'd read the values out instead and make sure they match what we pushed in
	//
	for (int i = 0 ; i < N ; i ++) {
		// When we decompress data we need to provide where to decode stuff to
		//
		las::point10 p2;
		decompressor.decompress((char *)&p2);

		// Finally make sure things match, otherwise bail
		if (p2.x != i ||
			p2.y != i + 1000 ||
			p2.z != i + 10000 ||
			p2.intensity != (unsigned short)(i + (1 << 15)) ||
			p2.return_number != ((i >> 3) & 0x7) ||
			p2.number_of_returns_of_given_pulse != (i & 0x7) ||
			p2.scan_direction_flag != (i & 1) ||
			p2.edge_of_flight_line != ((i+1) & 1) ||
			p2.classification != ((unsigned char) i % 256) ||
			p2.scan_angle_rank != ((unsigned char) i % 128) ||
			p2.user_data != ((i >> 4) % 256) ||
			p2.point_source_ID != ((i * 30) % (1 << 16)))
			throw std::runtime_error("Not matching");
	}

	// And we're done
	std::cout << "Done!" << std::endl;

	return 0;
}
