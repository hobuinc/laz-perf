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



void VlrCompressor::compress(const char *inbuf)
{
    // First time through.
    if (!m_encoder || !m_compressor)
    {
        // Get the position, which is 0 since we
        // are just starting to write
        m_chunkInfoPos = m_stream.m_buf.size();

        // Seek over the chunk info offset value
        unsigned char skip[sizeof(uint64_t)] = {0};
        m_stream.putBytes(skip, sizeof(skip));
        m_chunkOffset = m_offsetToData + m_chunkInfoPos + sizeof(uint64_t);

        resetCompressor();
    }
    else if (m_chunkPointsWritten == m_chunksize)
    {
        resetCompressor();
        newChunk();
    }
    m_compressor->compress(inbuf);
    m_chunkPointsWritten++;
}


//TODO HTOLE
void VlrCompressor::done()
{
    // Close and clear the point encoder.
    m_encoder->done();
    m_encoder.reset();

    newChunk();

    // Save our current position.  Go to the location where we need
    // to write the chunk table offset at the beginning of the point data.
    
    uint64_t chunkTablePos = htole64((uint64_t) m_stream.m_buf.size());
    // We need to add the offset given a construction time since
    // we did not use a stream to write the headers and vlrs
    uint64_t trueChunkTablePos = htole64((uint64_t) m_stream.m_buf.size() +  m_offsetToData);

    // Equivalent of stream.seekp(m_chunkInfoPos); stream << chunkTablePos
    memcpy(&m_stream.m_buf[m_chunkInfoPos], (char*) &trueChunkTablePos, sizeof(uint64_t));

    // Move to the start of the chunk table.
    // Which in our case is the end of the m_stream vector

    // Write the chunk table header in two steps
    // 1. Push bytes into the stream
    // 2. memcpy the data into the pushed bytes
    unsigned char skip[2 * sizeof(uint32_t)] = {0};
    m_stream.putBytes(skip, sizeof(skip));
    
    uint32_t version = htole32(0);
    uint32_t chunkTableSize = htole32((uint32_t) m_chunkTable.size());

    memcpy(&m_stream.m_buf[chunkTablePos], &version, sizeof(uint32_t));
    memcpy(&m_stream.m_buf[chunkTablePos + sizeof(uint32_t)], &chunkTableSize, sizeof(uint32_t));

    // Encode and write the chunk table.
    // OutputStream outputStream(m_stream);
    TypedLazPerfBuf<uint8_t> outputStream(m_stream);
    Encoder encoder(outputStream);
    laszip::compressors::integer compressor(32, 2);
    compressor.init();

    uint32_t predictor = 0;
    for (uint32_t offset : m_chunkTable)
    {
        offset = htole32(offset + m_offsetToData);
        compressor.compress(encoder, predictor, offset, 1);
        predictor = offset;
    }
    encoder.done();
}

void VlrCompressor::resetCompressor()
{
    if (m_encoder)
        m_encoder->done();
    m_encoder.reset(new Encoder(m_stream));
    m_compressor = laszip::factory::build_compressor(*m_encoder, m_schema);
}

void VlrCompressor::newChunk()
{
    size_t offset = m_stream.m_buf.size();
    m_chunkTable.push_back((uint32_t)(offset - m_chunkOffset));
    m_chunkOffset = offset;
    m_chunkPointsWritten = 0;
}

const std::vector<uint8_t>* VlrCompressor::data() const
{
    return &m_stream.m_buf;
}

} //Namespace


