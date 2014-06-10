// readlaz.cpp
// Read a LAZ file
//


#include "io.hpp"
#include "../common/common.hpp"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: readlaz <file.laz>" << std::endl;
		return -1;
	}

	std::string filename = argv[1];

	std::ifstream file(filename, std::ios::binary);
	laszip::io::reader::file f(file);

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
