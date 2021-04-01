// readlaz.cpp
// Read a LAZ file
//


#include "io.hpp"
#include "common.hpp"

#include <stdio.h>
#ifdef EMSCRIPTEN_BUILD
	#include <emscripten.h>
#endif

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: readlaz <file.laz>" << std::endl;
		return -1;
	}

	std::string filename = argv[1];

#ifdef EMSCRIPTEN_BUILD
	EM_ASM(
			FS.mkdir('/files');
			FS.mount(NODEFS, { root: '.' }, '/files');
	);

	filename = std::string("/files/" + filename);

	std::ifstream file(filename, std::ios::binary);
	if (!file.good()) {
		std::cerr << "Could not open file for reading: " << filename << std::endl;
		std::cerr << strerror(errno) << std::endl;
		return -1;
	}
#else
	std::ifstream file(filename, std::ios::binary);
	if (!file.good()) {
		std::cerr << "Could not open file for reading: " << filename << std::endl;
		std::cerr << strerror(errno) << std::endl;
		return -1;
	}
#endif


	lazperf::reader::generic_file f(file);

	size_t count = f.pointCount();
	char buf[256]; // a buffer large enough to hold our point

	auto start = common::tick();
	for(size_t i = 0 ; i < count ; i ++)
		f.readPoint(buf); // read the point out

	float t = common::since(start);

	std::cout << "Read through the points in " << t << " seconds." << std::endl;
	return 0;
}
