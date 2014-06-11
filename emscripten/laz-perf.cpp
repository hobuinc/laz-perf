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
			try {
				char *buf = (char*) b;
				int i = 0 ;

				for(size_t j = 0 ; j < len ; j ++) {
					if (j < 10)
						std::cout << j << ": " << buf[j] << std::endl;

					if (buf[j] == 0)
						i ++;
				}

				std::cout << "count: " << i << std::endl;

				pmem_stream_.reset(new stream_type(buf, len));
				pfile_.reset(new reader_type(*pmem_stream_));
			}
			catch(std::runtime_error& e) {
				std::cout << "Failed! " << e.what() << std::endl;
			}
			catch(...) {
				std::cout << "Unknown" << std::endl;
			}
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
