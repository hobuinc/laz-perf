// laz-perf.cpp
// javascript bindings for laz-perf
//

#include <emscripten/bind.h>
#include <iostream>

#include "header.hpp"
#include "readers.hpp"
#include "writers.hpp"

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
			return static_cast<unsigned int>(mem_file_->pointCount());
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

class ChunkDecoder
{
public:
    ChunkDecoder()
    {}

    void open(int pdrf, int point_length, unsigned int inputBuf)
    {
        int ebCount = point_length - lazperf::baseCount(pdrf);
        char *buf = reinterpret_cast<char *>(inputBuf);
        decomp_.reset(new lazperf::reader::chunk_decompressor(pdrf, ebCount, buf));
    }

    void getPoint(unsigned int outBuf)
    {
        char *buf = reinterpret_cast<char *>(outBuf);
        decomp_->decompress(buf);
    }

private:
    std::shared_ptr<lazperf::reader::chunk_decompressor> decomp_;
};

class LazWriter
{
public:
    LazWriter(): c()
    {}

    LazWriter& setPointFormat(int point_format)
    {
        c.pdrf = point_format;
        return *this;
    }

    LazWriter& setTransform(
        float offset_x,
        float offset_y,
        float offset_z,
        float scale_x,
        float scale_y,
        float scale_z
    )
    {
        c.scale = lazperf::vector3(scale_x, scale_y, scale_z);
        c.offset = lazperf::vector3(offset_x, offset_y, offset_z);
        return *this;
    }

    LazWriter& setChunkSize(uint32_t chunk_size)
    {
        c.chunk_size = chunk_size;
        return *this;
    }

    void open(
        unsigned int b,
        size_t len
    )
    {
        char *buf = (char*) b;
        mem_file_.reset(new lazperf::writer::mem_file(
            buf,
            len,
            c
        ));
    }

    void writePoint(unsigned int inputBuf)
    {
        char *buf = reinterpret_cast<char *>(inputBuf);
        mem_file_->writePoint(buf);
    }

    uint64_t newChunk()
    {
        return static_cast<uint64_t>(mem_file_->newChunk());
    }

    uint64_t close()
    {
        return static_cast<uint64_t>(mem_file_->close());
    }

private:
    std::shared_ptr<lazperf::writer::mem_file> mem_file_;
    lazperf::writer::named_file::config c;
};

EMSCRIPTEN_BINDINGS(my_module) {
	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open)
        .function("getPointLength", &LASZip::getPointLength)
        .function("getPointFormat", &LASZip::getPointFormat)
		.function("getPoint", &LASZip::getPoint)
		.function("getCount", &LASZip::getCount);

    class_<ChunkDecoder>("ChunkDecoder")
        .constructor()
        .function("open", &ChunkDecoder::open)
        .function("getPoint", &ChunkDecoder::getPoint);

    class_<LazWriter>("LazWriter")
        .constructor()
        .function("setPointFormat", &LazWriter::setPointFormat, return_value_policy::reference())
        .function("setTransform", &LazWriter::setTransform, return_value_policy::reference())
        .function("setChunkSize", &LazWriter::setChunkSize, return_value_policy::reference())
        .function("open", &LazWriter::open)
        .function("writePoint", &LazWriter::writePoint)
        .function("newChunk", &LazWriter::newChunk)
        .function("close", &LazWriter::close);
}

