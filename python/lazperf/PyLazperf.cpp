#include "PyLazperf.hpp"


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


Decompressor::Decompressor( std::vector<uint8_t>& compressed,
                            std::string const& json)
    : m_json(json)
    , m_stream(compressed)
    , m_decoder(m_stream)
    , m_decompressor(laszip::formats::make_dynamic_decompressor(m_decoder))
    , m_pointSize(0)
{
}

const char* Decompressor::getJSON() const
{
    return m_json.c_str();
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

}
