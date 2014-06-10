// laz-perf.cpp
// javascript bindings for laz-perf
//

#include <emscripten/bind.h>
#include <iostream>

#include "io.hpp"

using namespace emscripten;

struct MemoryStream {
	MemoryStream(const char *buf, size_t len) : buf_(buf), len_(len), offset_(0), is_bad_(false), last_read_count_(0) {
	}

	void read(char *into, size_t size) {
		if (size + offset_ >= len_)
			is_bad_ = true;
		else {
			size_t to_read = std::min(size, len_ - size);
			std::copy(buf_ + offset_, buf_ + offset_ + to_read, into);
			offset_ += to_read;
			last_read_count_ = to_read;
		}
	}

	bool eof() {
		return offset_ >= len_;
	}

	size_t gcount() {
		return last_read_count_;
	}

	bool good() {
		bool b = is_bad_;
		is_bad_ = false;
		return !b;
	}

	void clear() {
		is_bad_ = false;
	}

	void seekg(std::ios::pos_type p) {
		if (p > len_)
			is_bad_ = true;
		else
			offset_ = p;
	}

	void seekg(std::ios::off_type p, std::ios_base::seekdir dir) {
		size_t new_offset_ = 0;
		switch(dir) {
			case std::ios::beg: new_offset_ = p; break;
			case std::ios::end: new_offset_ = len_ - p; break;
			case std::ios::cur: new_offset_ = offset_ + p; break;
		}

		if (new_offset_ >= len_)
			is_bad_ = true;
		else {
			is_bad_ = false;
			offset_ = new_offset_;
		}
	}



	const char *buf_;
	size_t len_, offset_;
	bool is_bad_;
	size_t last_read_count_;
};

class LASZip {
	typedef laszip::io::reader::basic_file<MemoryStream> reader_type;
	public:
		LASZip() {}
		void open(unsigned int b, size_t len) {
			const char *buf = (const char*) b;

			pmem_stream_.reset(new MemoryStream(buf, len));
			pfile_.reset(new reader_type(*pmem_stream_));
		}

	private:
		std::shared_ptr<MemoryStream> pmem_stream_;
		std::shared_ptr<reader_type> pfile_;
};

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);

	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open, allow_raw_pointers());
}
