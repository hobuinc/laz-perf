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

#define PY_ARRAY_UNIQUE_SYMBOL LIBPDALPYTHON_ARRAY_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <numpy/arrayobject.h>


namespace pylazperf
{

template <typename CTYPE = unsigned char>
class TypedLazPerfBuf
{
    typedef std::vector<CTYPE> LazPerfRawBuf;
private:

    LazPerfRawBuf& m_buf;
    size_t m_idx;

public:
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

class Decompressor {
public:
    Decompressor(std::vector<uint8_t>&,
                 std::string const& schema_json);
    ~Decompressor(){};
    void add_dimension(pylazperf::Type t);
    size_t decompress(char* out, size_t buffer_size);
    inline size_t getPointSize() { return m_pointSize; }

    const char* getJSON() const;

private:

    typedef laszip::decoders::arithmetic<TypedLazPerfBuf<uint8_t>> Decoder;
    typedef typename laszip::formats::dynamic_field_decompressor<Decoder>::ptr
        Engine;

    std::string m_json;
    TypedLazPerfBuf<uint8_t> m_stream;
    Decoder m_decoder;
    Engine m_decompressor;
    size_t m_pointSize;
};

}
