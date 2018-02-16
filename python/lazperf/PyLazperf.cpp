#include "PyLazperf.hpp"
#include <stdexcept>


namespace pylazperf
{

template<typename LasZipEngine>
size_t addDimension(LasZipEngine& engine, pylazperf::Type t)
{

    switch (t)
    {
    case Type::Signed64:
        engine->template add_field<int32_t>();
        engine->template add_field<int32_t>();
        break;
    case Type::Signed32:
    case Type::Float:
        engine->template add_field<int32_t>();
        break;
    case Type::Signed16:
        engine->template add_field<int16_t>();
        break;
    case Type::Signed8:
        engine->template add_field<int8_t>();
        break;
    case Type::Unsigned64:
    case Type::Double:
        engine->template add_field<uint32_t>();
        engine->template add_field<uint32_t>();
        break;
    case Type::Unsigned32:
        engine->template add_field<uint32_t>();
        break;
    case Type::Unsigned16:
        engine->template add_field<uint16_t>();
        break;
    case Type::Unsigned8:
        engine->template add_field<uint8_t>();
        break;
    default:
        break;
    }
    return pylazperf::size(t);
}

LAZEngine::LAZEngine()
    : m_pointSize(0)
{
}


Decompressor::Decompressor( std::vector<uint8_t>& compressed)
    : LAZEngine()
    , m_stream(compressed)
    , m_decoder(m_stream)
    , m_decompressor(laszip::formats::make_dynamic_decompressor(m_decoder))
{
}

void Decompressor::add_dimension(pylazperf::Type t)
{
    m_pointSize = m_pointSize + addDimension(m_decompressor, t);
}

size_t Decompressor::decompress(char* output, size_t buffer_size)
{
    size_t count (0);

    char* out = output;
    char* end = out + buffer_size;

    while (out + m_pointSize <= end)
    {
        // Determine when we're done?
        m_decompressor->decompress(out);
        out += m_pointSize;
        count++;
    }
    return count;

}


Compressor::Compressor( std::vector<uint8_t>& uncompressed)
    : LAZEngine()
    , m_stream(uncompressed)
    , m_encoder(m_stream)
    , m_compressor(laszip::formats::make_dynamic_compressor(m_encoder))
    , m_done(false)
{
}

void Compressor::done()
{
    if (!m_done)
       m_encoder.done();
    m_done = true;
}

size_t Compressor::compress(const char *inbuf, size_t bufsize)
{
    size_t numRead = 0;

    if (m_done)
        throw std::runtime_error("Encoder has finished, unable to compress!");

    const char *end = inbuf + bufsize;
    while (inbuf + m_pointSize <= end)
    {
        m_compressor->compress(inbuf);
        inbuf += m_pointSize;
        numRead++;
    }
    return numRead;
}

void Compressor::add_dimension(pylazperf::Type t)
{
    m_pointSize = m_pointSize + addDimension(m_compressor, t);
}

const std::vector<uint8_t>* Compressor::data() const
{
    return &m_stream.m_buf;
}

} //Namespace


