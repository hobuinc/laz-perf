/*
===============================================================================

  FILE:  io.cpp

  CONTENTS:
    LAZ io

  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
    uday.karan@gmail.com - Hobu, Inc.

  COPYRIGHT:

    (c) 2007-2014, martin isenburg, rapidlasso - tools to catch reality
    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  CHANGE HISTORY:

===============================================================================
*/

#include "writers.hpp"
#include "writers_private.hpp"

namespace lazperf
{
namespace writer
{

// On compressed output, the data is written in chunks. Normally the chunks are 50,000
// points, but you can request variable sized chunking by using the value "VariableChunkSize".
// When using variable-sized chunks, you must call chunk() in order to start a new chunk.
//
// An offset to the chunk table is written after the LAS VLRs, where you would normally
// expect points to begin.  The first chunk of points follows this chunk table offset.
//
// The chunk table is at the end of the points. It has its own
// header that consists of a version number and a chunk count. The chunk table entries
// are compressed with an integer compressor.  If the chunks are variable-sized,
// the first item of each chunk table entry is a count of the number of points in
// the chunk. If the chunks are fixed sized, this value is omitted (not written).
// Every chunk entry contains an "offset". It's important to note that this value
// is a relative offset to the *next* chunk.  The offset to the first chunk isn't
// written and is instead calculated using the point offset in the base header.
// This strange arrangment is fixed-up when the chunk table is read into
// memory and the offset becomes an absolute offset to the chunk which contains
// the number of points stored in the entry.

void basic_file::Private::open(std::ostream& out, const io::base_header& h, uint32_t cs)
{
    header = h;
    chunk_size = cs;
    f = &out;

    size_t preludeSize = header.size();
    if (h.version.minor == 3)
    {
        header13 = (const io::header13&)h;
        preludeSize = sizeof(io::header13);
    }
    else if (h.version.minor == 4)
    {
        header14 = (const io::header14&)h;
        preludeSize = sizeof(io::header14);
    }

    if (compressed())
    {
        preludeSize += sizeof(int64_t);  // Chunk table offset.
        laz_vlr vlr(h.point_format_id, h.ebCount(), chunk_size);
        preludeSize += vlr.size() + vlr.header().size();
    }
    if (h.ebCount())
    {
        eb_vlr vlr(h.ebCount());
        preludeSize += vlr.size() + vlr.header().size();
    }

    std::vector<char> junk(preludeSize);
    f->write(junk.data(), preludeSize);
    stream.reset(new OutFileStream(out));
}

bool basic_file::Private::compressed() const
{
    return chunk_size > 0;
}

void basic_file::Private::updateMinMax(const las::point10& p)
{
    io::base_header& h = header;

    double x = p.x * h.scale.x + h.offset.x;
    double y = p.y * h.scale.y + h.offset.y;
    double z = p.z * h.scale.z + h.offset.z;

    h.minimum.x = (std::min)(x, h.minimum.x);
    h.minimum.y = (std::min)(y, h.minimum.y);
    h.minimum.z = (std::min)(z, h.minimum.z);

    h.maximum.x = (std::max)(x, h.maximum.x);
    h.maximum.y = (std::max)(y, h.maximum.y);
    h.maximum.z = (std::max)(z, h.maximum.z);
}

uint64_t basic_file::Private::chunk()
{
    pcompressor->done();

    uint64_t position = (uint64_t)f->tellp();
    chunks.push_back({ chunk_point_num, position });
    pcompressor = build_las_compressor(stream->cb(), header.point_format_id, header.ebCount());
    chunk_point_num = 0;
    return position;
}

uint64_t basic_file::Private::firstChunkOffset() const
{
    // There is a chunk offset where the first point is supposed to be. The first
    // chunk follows that.
    return header.point_offset + sizeof(uint64_t);
}

void basic_file::Private::writePoint(const char *p)
{
    if (!compressed())
        stream->putBytes(reinterpret_cast<const unsigned char *>(p), header.point_record_length);
    else
    {
        //ABELL - This first bit can go away if we simply always create compressor.
        if (!pcompressor)
        {
            pcompressor = build_las_compressor(stream->cb(), header.point_format_id,
                header.ebCount());
            chunk_point_num = 0;
        }
        else if ((chunk_point_num == chunk_size) && (chunk_size != io::VariableChunkSize))
            chunk();

        // now write the point
        pcompressor->compress(p);
        chunk_point_num++;
        header14.point_count_14++;
    }
    updateMinMax(*(reinterpret_cast<const las::point10*>(p)));
}

void basic_file::Private::close()
{
    if (compressed())
    {
        pcompressor->done();
        chunks.push_back({ chunk_point_num, (uint64_t)f->tellp() });
    }

    writeHeader();
    if (compressed())
        writeChunkTable();
}

void basic_file::Private::writeHeader()
{
    io::base_header& h = header;

    laz_vlr lazVlr(h.point_format_id, h.ebCount(), chunk_size);
    eb_vlr ebVlr(h.ebCount());

    // point_format_id and point_record_length  are set on open().
    h.header_size = h.size();
    h.point_offset = h.header_size;
    h.vlr_count = 0;
    if (compressed())
    {
       h.point_offset += lazVlr.size() + lazVlr.header().size();
       h.vlr_count++;
       h.point_format_id |= (1 << 7);
    }
    if (h.ebCount())
    {
        h.point_offset += ebVlr.size() + ebVlr.header().size();
        h.vlr_count++;
    }

    //HUH?
    // make sure we re-arrange mins and maxs for writing
    double mx, my, mz, nx, ny, nz;
    nx = h.minimum.x; mx = h.maximum.x;
    ny = h.minimum.y; my = h.maximum.y;
    nz = h.minimum.z; mz = h.maximum.z;

    h.minimum.x = mx; h.minimum.y = nx;
    h.minimum.z = my; h.maximum.x = ny;
    h.maximum.y = mz; h.maximum.z = nz;

    if (h.version.minor == 4)
    {
        if (header14.point_count_14 > (std::numeric_limits<uint32_t>::max)())
            h.point_count = 0;
        else
            h.point_count = (uint32_t)header14.point_count_14;
        // Set the WKT bit.
        h.global_encoding |= (1 << 4);
    }
    else
        h.point_count = (uint32_t)header14.point_count_14;

    f->seekp(0);
    f->write(reinterpret_cast<char*>(&h), h.header_size);

    if (compressed())
    {
        // Write the VLR.
        vlr::vlr_header vh = lazVlr.header();
        f->write(reinterpret_cast<char *>(&vh), sizeof(vh));

        std::vector<char> vlrbuf = lazVlr.data();
        f->write(vlrbuf.data(), vlrbuf.size());
    }
    if (h.ebCount())
    {
        vlr::vlr_header vh = ebVlr.header();
        f->write(reinterpret_cast<char *>(&vh), sizeof(vh));

        std::vector<char> vlrbuf = ebVlr.data();
        f->write(vlrbuf.data(), vlrbuf.size());
    }
}

void basic_file::Private::writeChunkTable()
{
    // move to the end of the file to start emitting our compresed table
    f->seekp(0, std::ios::end);

    // take note of where we're writing the chunk table, we need this later
    int64_t chunk_table_offset = static_cast<int64_t>(f->tellp());
    // write out the chunk table header (version and total chunks)
#pragma pack(push, 1)
    struct
    {
        unsigned int version,
        chunks_count;
    } chunk_table_header = { 0, static_cast<unsigned int>(chunks.size()) };
#pragma pack(pop)

    f->write(reinterpret_cast<char*>(&chunk_table_header), sizeof(chunk_table_header));

    // Now compress and write the chunk table
    OutFileStream w(*f);
    OutCbStream outStream(w.cb());

    encoders::arithmetic<OutCbStream> encoder(outStream);
    compressors::integer comp(32, 2);

    comp.init();

    Chunk prevChunk { 0, firstChunkOffset() };
    uint32_t last_count = 0;
    int32_t last_offset = 0;
    for (size_t i = 0; i < chunks.size(); i++)
    {
        Chunk& chunk = chunks[i];
        if (chunk_size == io::VariableChunkSize)
        {
            uint32_t count = (uint32_t)(chunk.count);
            comp.compress(encoder, last_count, count, 0);
            last_count = count;
        }

        // Turn the absolute offsets into relative offsets.
        uint32_t offset = (uint32_t)(chunk.offset - prevChunk.offset);
        comp.compress(encoder, last_offset, offset, 1);
        last_offset = offset;

        prevChunk = chunk;
    }
    encoder.done();

    // go back to where we're supposed to write chunk table offset
    f->seekp(header.point_offset);
    f->write(reinterpret_cast<char*>(&chunk_table_offset), sizeof(chunk_table_offset));
}


basic_file::basic_file() : p_(new Private())
{}

basic_file::~basic_file()
{}

bool basic_file::compressed() const
{
    return p_->compressed();
}

void basic_file::open(std::ostream& out, const io::base_header& h, uint32_t chunk_size)
{
    p_->open(out, h, chunk_size);
}

void basic_file::writePoint(const char *buf)
{
    p_->writePoint(buf);
}

uint64_t basic_file::firstChunkOffset() const
{
    return p_->firstChunkOffset();
}

uint64_t basic_file::chunk()
{
    assert(p_->chunk_size == io::VariableChunkSize);
    return p_->chunk();
}

void basic_file::close()
{
    p_->close();
}

// named_file

named_file::config::config() : scale(1.0, 1.0, 1.0), offset(0.0, 0.0, 0.0),
    chunk_size(io::DefaultChunkSize), pdrf(0), minor_version(3), extra_bytes(0)
{}

named_file::config::config(const io::vector3& s, const io::vector3& o, unsigned int cs) :
    scale(s), offset(o), chunk_size(cs), pdrf(0), minor_version(3), extra_bytes(0)
{}

named_file::config::config(const io::base_header& h) : scale(h.scale.x, h.scale.y, h.scale.z),
    offset(h.offset.x, h.offset.y, h.offset.z), chunk_size(io::DefaultChunkSize),
    pdrf(h.point_format_id), extra_bytes(h.ebCount())
{}

io::base_header named_file::config::to_header() const
{
    io::base_header h;

    h.minimum = { (std::numeric_limits<double>::max)(), (std::numeric_limits<double>::max)(),
        (std::numeric_limits<double>::max)() };
    h.maximum = { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()};
    h.version.minor = minor_version;
    h.point_format_id = pdrf;
    h.point_record_length = baseCount(pdrf) + extra_bytes;

    h.offset.x = offset.x;
    h.offset.y = offset.y;
    h.offset.z = offset.z;

    h.scale.x = scale.x;
    h.scale.y = scale.y;
    h.scale.z = scale.z;

    return h;
}

void named_file::Private::open(const std::string& filename, const named_file::config& c)
{
    io::base_header h = c.to_header();

    // open the file and move to offset of data, we'll write
    // headers and all other things on file close
    f.open(filename, std::ios::binary | std::ios::trunc);
    if (!f.good())
        throw error("Couldn't open '" + filename + "' for writing.");
    base->open(f, h, c.chunk_size);
}


named_file::named_file(const std::string& filename, const named_file::config& c) :
    p_(new Private(basic_file::p_.get()))
{
    p_->open(filename, c);
}    

named_file::~named_file()
{}

void named_file::close()
{
    basic_file::close();
    if (p_->f.is_open())
        p_->f.close();
}

} // namespace writer
} // namespace lazperf

