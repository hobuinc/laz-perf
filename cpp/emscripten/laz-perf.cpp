// laz-perf.cpp
// javascript bindings for laz-perf
//

#include <emscripten/bind.h>
#include <iostream>

#include "readers.hpp"

using namespace emscripten;

class LASZip
{
	public:
		LASZip()
        {}

		void open(unsigned int b, size_t len)
        {
			char *buf = (char*) b;
			mem_file_.reset(new lazperf::reader::mem_file(buf, len));
		}

		void getPoint(int buf)
        {
			char *pbuf = reinterpret_cast<char*>(buf);
			mem_file_->readPoint(pbuf);
		}

		unsigned int getCount() const
        {
			return static_cast<unsigned int>(mem_file_->header().point_count);
		}

        unsigned int getPointLength() const
        {
			return static_cast<unsigned int>(mem_file_->header().point_record_length);
        }

        unsigned int getPointFormat() const
        {
			return static_cast<unsigned int>(mem_file_->header().point_format_id);
        }

	private:
		std::shared_ptr<lazperf::reader::mem_file> mem_file_;
};

EMSCRIPTEN_BINDINGS(my_module) {
	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open)
        .function("getPointLength", &LASZip::getPointLength)
        .function("getPointFormat", &LASZip::getPointFormat)
		.function("getPoint", &LASZip::getPoint)
		.function("getCount", &LASZip::getCount);
}
