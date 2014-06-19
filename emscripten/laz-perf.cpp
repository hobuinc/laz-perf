// laz-perf.cpp
// javascript bindings for laz-perf
//

#include <emscripten/bind.h>
#include <iostream>

#include "streams.hpp"
#include "io.hpp"

using namespace emscripten;

class LASZip {
	typedef laszip::streams::memory_stream stream_type;
	typedef laszip::io::reader::basic_file<stream_type> reader_type;
	public:
		LASZip() : pmem_stream_(), pfile_() {}
		~LASZip() {}

		void open(unsigned int b, size_t len) {
			char *buf = (char*) b;

			std::cout << "Got data " << len << " bytes" << std::endl;

			pmem_stream_.reset(new stream_type(buf, len));
			pfile_.reset(new reader_type(*pmem_stream_));
		}

		void getPoint(int buf) {
			char *pbuf = reinterpret_cast<char*>(buf);
			pfile_->readPoint(pbuf);
		}

	private:
		std::shared_ptr<stream_type> pmem_stream_;
		std::shared_ptr<reader_type> pfile_;
};

EMSCRIPTEN_BINDINGS(my_module) {
	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open, allow_raw_pointers())
		.function("getPoint", &LASZip::getPoint);
}
