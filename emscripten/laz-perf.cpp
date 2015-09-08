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
		~LASZip() {
            pmem_stream_.reset();
            pfile_.reset();
        }

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

		unsigned int getCount() {
			return static_cast<unsigned int>(pfile_->get_header().point_count);
		}

	private:
		std::shared_ptr<stream_type> pmem_stream_;
		std::shared_ptr<reader_type> pfile_;
};

/** FRIENDLY REMINDER: If you go over with buffer reads, its your own fault */
struct buf_stream {
    buf_stream(unsigned char *b, size_t s) :buf(b), size(s), _i(0) {}

    unsigned char getByte() { return buf[_i++]; };
    void getBytes(unsigned char *b, int len) {
        for (int i = 0 ; i < len ; i ++) {
            b[i] = getByte();
        }
    }

    unsigned char *buf;
    size_t size, _i;
};

class DynamicLASZip {
	typedef buf_stream                                            stream_type;
    typedef laszip::decoders::arithmetic<stream_type>             decoder_type;
    typedef laszip::formats::dynamic_field_decompressor<decoder_type>::ptr decompressor_ptr;
public:
    DynamicLASZip() {}
    ~DynamicLASZip() {
        pmem_stream_.reset();
        pdecomp_.reset();
        pdecomp_.reset();
    }

    void addFieldFloating(size_t size) {
        if (!pdecomp_)
          return;

        switch(size) {
        case 4: pdecomp_->add_field<int32_t>(); break;
        case 8: pdecomp_->add_field<uint32_t>(); pdecomp_->add_field<uint32_t>(); break;
        }
    }

    void addFieldSigned(size_t size) {
        if (!pdecomp_)
          return;

        switch(size) {
        case 1: pdecomp_->add_field<int8_t>(); break;
        case 2: pdecomp_->add_field<int16_t>(); break;
        case 8: pdecomp_->add_field<int32_t>();
        case 4: pdecomp_->add_field<int32_t>(); break;
        }
    }

    void addFieldUnsigned(size_t size) {
        if (!pdecomp_)
          return;

        switch(size) {
        case 1: pdecomp_->add_field<uint8_t>(); break;
        case 2: pdecomp_->add_field<uint16_t>(); break;
        case 8: pdecomp_->add_field<uint32_t>();
        case 4: pdecomp_->add_field<uint32_t>(); break;
        }
    }

    void open(unsigned int b, size_t len) {
        // open ze stream
        //
        unsigned char *buf = reinterpret_cast<unsigned char*>(b);
        pmem_stream_.reset(new stream_type(buf, len));

        // open ze decoder
        pdecoder_.reset(new decoder_type(*pmem_stream_));

        // finally open the decompressor
        pdecomp_ = laszip::formats::make_dynamic_decompressor(*pdecoder_);
    }

    void getPoint(int buf) {
        char *b = reinterpret_cast<char *>(buf);
        if (pdecomp_)
          pdecomp_->decompress(b);
    }

private:
    std::shared_ptr<stream_type>   pmem_stream_;
    std::shared_ptr<decoder_type>  pdecoder_;
    decompressor_ptr               pdecomp_;
};

EMSCRIPTEN_BINDINGS(my_module) {
	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open, allow_raw_pointers())
		.function("getPoint", &LASZip::getPoint)
		.function("getCount", &LASZip::getCount);

    class_<DynamicLASZip>("DynamicLASZip")
        .constructor()
		.function("open", &DynamicLASZip::open, allow_raw_pointers())
        .function("addFieldFloating", &DynamicLASZip::addFieldFloating)
        .function("addFieldSigned", &DynamicLASZip::addFieldSigned)
        .function("addFieldUnsigned", &DynamicLASZip::addFieldUnsigned)
		.function("getPoint", &DynamicLASZip::getPoint);
}
