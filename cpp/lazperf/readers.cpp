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

#include "readers.hpp"
#include "readers_private.hpp"

namespace lazperf
{
namespace reader
{

// reader::basic_file

void basic_file::Private::open(std::istream& in)
{
    f = &in;
    //ABELL - move to loadHeader() in order to avoid the reset on InFileStream.
    stream.reset(new InFileStream(in));
    loadHeader();
}

uint64_t basic_file::Private::firstChunkOffset() const
{
    // There is a chunk offset where the first point is supposed to be. The first
    // chunk follows that.
    return header.point_offset + sizeof(uint64_t);
}

void basic_file::Private::readPoint(char *out)
{
    if (!compressed)
        stream->cb()(reinterpret_cast<unsigned char *>(out), header.point_record_length);

    // read the next point
    else
    {
        if (!pdecompressor || chunk_point_num == current_chunk->count)
        {
            pdecompressor = build_las_decompressor(stream->cb(), header.point_format_id,
                header.ebCount());

            // reset chunk state
            if (current_chunk == nullptr)
                current_chunk = chunks.data();
            else
                current_chunk++;
            chunk_point_num = 0;
        }

        pdecompressor->decompress(out);
        chunk_point_num++;
    }
}

void basic_file::Private::loadHeader()
{
    // Make sure our header is correct
    char magic[4];
    f->read(magic, sizeof(magic));

    if (std::string(magic, magic + 4) != "LASF")
        throw error("Invalid LAS file. Incorrect magic number.");

    // Read the header in
    f->seekg(0);
    f->read((char*)&header, sizeof(io::base_header));
    // If we're version 4, back up and do it again.
    if (header.version.minor == 3)
    {
        f->seekg(0);
        f->read((char *)&header13, sizeof(io::header13));
    }
    else if (header.version.minor == 4)
    {
        f->seekg(0);
        f->read((char *)&header14, sizeof(io::header14));
    }

    if (header.point_format_id & 0x80)
        compressed = true;

    // The mins and maxes are in a weird order, fix them
    fixMinMax();

    parseVLRs();

    if (compressed)
    {
        validateHeader();
        parseChunkTable();
    }

    // set the file pointer to the beginning of data to start reading
    // may have treaded past the EOL, so reset everything before we start reading
    f->clear();
    uint64_t offset = header.point_offset;
    if (compressed)
        offset += sizeof(int64_t);
    f->seekg(offset);
    stream->reset();
}


uint64_t basic_file::Private::pointCount() const
{
    if (header.version.major > 1 || header.version.minor > 3)
        return header14.point_count_14;
    return header.point_count;
}


void basic_file::Private::fixMinMax()
{
    double mx, my, mz, nx, ny, nz;

    io::base_header& h = header;
    mx = h.minimum.x; nx = h.minimum.y;
    my = h.minimum.z; ny = h.maximum.x;
    mz = h.maximum.y; nz = h.maximum.z;

    h.minimum.x = nx; h.maximum.x = mx;
    h.minimum.y = ny; h.maximum.y = my;
    h.minimum.z = nz; h.maximum.z = mz;
}

void basic_file::Private::parseVLRs()
{
    // move the pointer to the begining of the VLRs
    f->seekg(header.header_size);

#pragma pack(push, 1)
    struct {
        unsigned short reserved;
        char user_id[16];
        unsigned short record_id;
        unsigned short record_length;
        char desc[32];
    } vlr_header;
#pragma pack(pop)

    size_t count = 0;
    bool laszipFound = false;
    while (count < header.vlr_count && f->good() && !f->eof())
    {
        f->read((char*)&vlr_header, sizeof(vlr_header));

        const char *user_id = "laszip encoded";

        if (std::equal(vlr_header.user_id, vlr_header.user_id + 14, user_id) &&
            vlr_header.record_id == 22204)
        {
            laszipFound = true;

            std::unique_ptr<char> buffer(new char[vlr_header.record_length]);
            f->read(buffer.get(), vlr_header.record_length);
            parseLASZIPVLR(buffer.get());
            break; // no need to keep iterating
        }
        f->seekg(vlr_header.record_length, std::ios::cur); // jump foward
        count++;
    }

    if (compressed && !laszipFound)
        throw error("Couldn't find LASZIP VLR");
}

void basic_file::Private::parseLASZIPVLR(const char *buf)
{
    laz.fill(buf);
    if (laz.compressor != 2)
        throw error("LASZIP format unsupported - invalid compressor version.");
}

void basic_file::Private::parseChunkTable()
{
    // Move to the begining of the data
    f->seekg(header.point_offset);

    int64_t chunkoffset = 0;
    f->read((char*)&chunkoffset, sizeof(chunkoffset));
    if (!f->good())
        throw error("Couldn't read chunk table.");

    if (chunkoffset == -1)
        throw error("Chunk table offset == -1 is not supported at this time");

    // Go to the chunk offset and read in the table
    f->seekg(chunkoffset);
    if (!f->good())
        throw error("Error reading chunk table.");

    // Now read in the chunk table
#pragma pack(push, 1)
    struct
    {
        uint32_t version;
        uint32_t chunk_count;
    } chunk_table_header;
#pragma pack(pop)

    f->read((char *)&chunk_table_header, sizeof(chunk_table_header));
    if (!f->good())
        throw error("Error reading chunk table.");

    if (chunk_table_header.version != 0)
        throw error("Bad chunk table. Invalid version.");

    // Allocate enough room for the chunk table plus one because of the crazy way that
    // the chunk table is written. Once it is fixed up, we resize back to the correct size.
    chunks.resize(chunk_table_header.chunk_count + 1);

    // decode the index out
    InFileStream fstream(*f);

    InCbStream stream(fstream.cb());
    decoders::arithmetic<InCbStream> decoder(stream);
    decompressors::integer decomp(32, 2);

    // start decoder
    decoder.readInitBytes();
    decomp.init();

    uint32_t prev_count = 0;
    uint32_t prev_offset = 0;
    uint64_t total_points = pointCount();
    chunks[0] = { 0, firstChunkOffset() };
    for (size_t i = 0; i < chunk_table_header.chunk_count; i++)
    {
        uint32_t count;

        if (laz.chunk_size == io::VariableChunkSize)
        {
            count = decomp.decompress(decoder, prev_count, 0);
            prev_count = count;
        }
        else
        {
            if (total_points < laz.chunk_size)
            {
                count = total_points;
                assert(i == chunk_table_header.chunk_count - 1);
            }
            else
            {
                count = laz.chunk_size;
                total_points -= laz.chunk_size;
            }
        }

        uint32_t offset = decomp.decompress(decoder, prev_offset, 1);
        prev_offset = offset;

        chunks[i].count = count;
        chunks[i + 1].offset = offset + chunks[i].offset;
    }

    // This discards the last offset, which we don't care about. The last count is
    // never filled in.
    chunks.resize(chunk_table_header.chunk_count);
    /**
    for (auto& chunk : chunks)
        std::cerr << "Count/offset = " << chunk.count << "/" << chunk.offset << "!\n";
    std::cerr << "\n";
    **/
}

void basic_file::Private::validateHeader()
{
    io::base_header& h = header;

    int bit_7 = (h.point_format_id >> 7) & 1;
    int bit_6 = (h.point_format_id >> 6) & 1;

    if (bit_7 == 1 && bit_6 == 1)
        throw error("Header bits indicate unsupported old-style compression.");
    if ((bit_7 ^ bit_6) == 0)
        throw error("Header indicates the file is not compressed.");
    h.point_format_id &= 0x3f;
}


basic_file::basic_file() : p_(new Private)
{}

basic_file::~basic_file()
{}

void basic_file::open(std::istream& f)
{
    p_->open(f);
}

void basic_file::readPoint(char *out)
{
    p_->readPoint(out);
}

const io::base_header& basic_file::header() const
{
    return p_->header;
}

uint64_t basic_file::pointCount() const
{
    return p_->pointCount();
}

// reader::mem_file

mem_file::mem_file(char *buf, size_t count) : p_(new Private(buf, count))
{
    open(p_->f);
}

mem_file::~mem_file()
{}

// reader::generic_file

generic_file::~generic_file()
{}

generic_file::generic_file(std::istream& in)
{
    open(in);
}

// reader::named_file

named_file::named_file(const std::string& filename) : p_(new Private(filename))
{
    open(p_->f);
}

named_file::~named_file()
{}

} // namespace reader
} // namespace lazperf

