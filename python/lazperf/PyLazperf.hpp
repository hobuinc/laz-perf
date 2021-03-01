#pragma once

#include <laz-perf/common/common.hpp>
#include <laz-perf/compressor.hpp>
#include <laz-perf/decompressor.hpp>

#include <laz-perf/encoder.hpp>
#include <laz-perf/decoder.hpp>
#include <laz-perf/formats.hpp>
#include <laz-perf/io.hpp>
#include <laz-perf/las.hpp>

#include <string>
#include <vector>

#include "PyLazperfTypes.hpp"


namespace pylazperf
{

template <typename CTYPE = unsigned char>
class TypedLazPerfBuf
{
    typedef std::vector<CTYPE> LazPerfRawBuf;

public:
    LazPerfRawBuf& m_buf;
    size_t m_idx;

    TypedLazPerfBuf(LazPerfRawBuf& buf) : m_buf(buf), m_idx(0)
    {}

    void putBytes(const unsigned char *b, size_t len)
    {
        m_buf.insert(m_buf.end(), (const CTYPE *)b, (const CTYPE *)(b + len));
    }
    void putByte(const unsigned char b)
        {   m_buf.push_back((CTYPE)b); }
    unsigned char getByte()
        { return (unsigned char)m_buf[m_idx++]; }
    void getBytes(unsigned char *b, int len)
    {
        memcpy(b, m_buf.data() + m_idx, len);
        m_idx += len;
    }
};

/*
 * Provides a read only wrapper around a buffer to allow lazperf's decoders to read from it.
 * This is used to avoid having to copy uncompressed points data (potentially huge)
 * from a numpy array to a std::vector
*/
class ReadOnlyStream
{
public:
    const uint8_t *m_data;
    size_t m_dataLength;
    size_t m_idx;

    ReadOnlyStream(const uint8_t *data, size_t dataLen)
    : m_data(data)
    , m_dataLength(dataLen)
    , m_idx(0)
    {}


    unsigned char getByte()
    {
        if (m_idx >= m_dataLength) {
            throw std::runtime_error("Tried to read past buffer bounds");
        }
        return (unsigned char) m_data[m_idx++];
    }

    void getBytes(unsigned char *b, int len)
    {
        if (m_idx + len > m_dataLength) {
            throw std::runtime_error("Tried to read past buffer bounds");
        }
        memcpy(b, (unsigned char*) &m_data[m_idx], len);
        m_idx += len;
    }
};

class LAZEngine
{
public:
    LAZEngine();
    virtual ~LAZEngine() {};
    virtual size_t getPointSize() const { return m_pointSize; }

protected:
    size_t m_pointSize;
};

class Decompressor : public LAZEngine {
public:
    Decompressor(const uint8_t *data, size_t dataLen);
    ~Decompressor(){};
    void add_dimension(pylazperf::Type t);
    size_t decompress(char* out, size_t buffer_size);


private:

    typedef laszip::decoders::arithmetic<ReadOnlyStream> Decoder;
    typedef typename laszip::formats::dynamic_field_decompressor<Decoder>::ptr
        Engine;

    ReadOnlyStream m_stream;
    Decoder m_decoder;
    Engine m_decompressor;
};


class Compressor : public LAZEngine
{
public:
    Compressor(std::vector<uint8_t>&);
    ~Compressor(){};
    void done();
    void add_dimension(pylazperf::Type t);
    size_t compress(const char* input, size_t buffer_size);
    const std::vector<uint8_t>* data() const;
    void copy_data_to(uint8_t *destination) const;

private:

    typedef laszip::encoders::arithmetic<TypedLazPerfBuf<uint8_t>> Encoder;
    typedef typename laszip::formats::dynamic_field_compressor<Encoder>::ptr Engine;

    TypedLazPerfBuf<uint8_t> m_stream;
    Encoder m_encoder;
    Engine m_compressor;
    bool m_done;
};


class VlrDecompressor
{
public:
    VlrDecompressor(
        const uint8_t *compressedData,
        size_t dataLength,
        size_t pointSize,
        const char *vlr_data)
        : m_stream(compressedData, dataLength)
        , m_chunksize(0)
        , m_chunkPointsRead(0)
    {
        laszip::io::laz_vlr zipvlr(vlr_data);
        m_chunksize = zipvlr.chunk_size;
        m_schema = laszip::io::laz_vlr::to_schema(zipvlr, pointSize);
    }

    size_t getPointSize() const
        { return (size_t)m_schema.size_in_bytes(); }

    void decompress(char* out)
    {
        if (m_chunkPointsRead == m_chunksize || !m_decoder || !m_decompressor)
        {
            resetDecompressor();
            m_chunkPointsRead = 0;
        }
        m_decompressor->decompress(out);
        m_chunkPointsRead++;
    }

private:
    void resetDecompressor()
    {
        m_decoder.reset(new Decoder(m_stream));
        m_decompressor =
            laszip::factory::build_decompressor(*m_decoder, m_schema);
    }


    typedef laszip::formats::dynamic_decompressor Decompressor;
    typedef laszip::factory::record_schema Schema;
    typedef laszip::decoders::arithmetic<ReadOnlyStream> Decoder;

    ReadOnlyStream m_stream;

    std::unique_ptr<Decoder> m_decoder;
    Decompressor::ptr m_decompressor;
    Schema m_schema;
    uint32_t m_chunksize;
    uint32_t m_chunkPointsRead;
};



typedef laszip::factory::record_schema Schema;
class VlrCompressor
{
public:
    VlrCompressor(Schema s, uint64_t offset_to_data)
    : m_stream(m_data_vec)
    , m_encoder(nullptr)
    , m_chunkPointsWritten(0)
    , m_offsetToData(offset_to_data)
    , m_chunkInfoPos(0)
    , m_chunkOffset(0)
    , m_schema(s)
    , m_vlr(laszip::io::laz_vlr::from_schema(m_schema))
    , m_chunksize(m_vlr.chunk_size)
    {
    }

    const std::vector<uint8_t>* data() const;
    void compress(const char *inbuf);
    void done();

    size_t vlrDataSize() const
        { return m_vlr.size(); }

    void extractVlrData(char *out_data)
        { return m_vlr.extract(out_data); }

    size_t getPointSize() const
        { return (size_t)m_schema.size_in_bytes(); }

    void copy_data_to(uint8_t *dst) const
        { std::copy(m_data_vec.begin(), m_data_vec.end(), dst); }


private:
    typedef laszip::encoders::arithmetic<TypedLazPerfBuf<uint8_t>> Encoder;
    typedef laszip::formats::dynamic_compressor Compressor;

    void resetCompressor();
    void newChunk();

    std::vector<uint8_t> m_data_vec;
    TypedLazPerfBuf<uint8_t> m_stream;
    std::unique_ptr<Encoder> m_encoder;
    Compressor::ptr m_compressor;
    uint32_t m_chunkPointsWritten;
    uint64_t m_offsetToData;
    uint64_t m_chunkInfoPos;
    uint64_t m_chunkOffset;
    Schema m_schema;
    laszip::io::laz_vlr m_vlr;
    uint32_t m_chunksize;
    std::vector<uint32_t> m_chunkTable;
};


} // pylazperf namespace
