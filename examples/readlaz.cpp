// readlaz.cpp
// Read a LAZ file
//


#include "io.hpp"
#include "../common/common.hpp"

int main() {
	laszip::io::file f("test/raw-sets/autzen.laz");

	size_t count = f.get_header().point_count;
	char buf[256]; // a buffer large enough to hold our point

	auto start = common::tick();
	for(size_t i = 0 ; i < count ; i ++) {
		f.readPoint(buf); // read the point out
	}

	float t = common::since(start);

	std::cout << "Read through the points in " << t << " seconds." << std::endl;
	return 0;
}
