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

#include "io.hpp"

namespace lazperf
{

namespace
{

int baseCount(int format)
{
    // Martin screws with the high bits of the format, so we mask down to the low four bits.
    switch (format & 0xF)
    {
    case 0:
        return 20;
    case 1:
        return 28;
    case 2:
        return 26;
    case 3:
        return 34;
    case 6:
        return 30;
    case 7:
        return 36;
    case 8:
        return 38;
    default:
        return 0;
    }
}

} // unnamed namespace

int io::header::ebCount() const
{
    int baseSize = baseCount(point_format_id);
    return (baseSize ? point_record_length - baseSize : 0);
}

namespace reader
{

basic_file::basic_file() : header_(header14_), compressed_(false)
{}

basic_file::~basic_file()
{}

void basic_file::open(std::istream& f)
{
    f_ = &f;
    //ABELL - move to loadHeader() in order to avoid the reset on InFileStream.
    stream_.reset(new InFileStream(f));
    loadHeader();
}

void basic_file::readPoint(char *out)
{
    if (!compressed_)
        stream_->cb()(reinterpret_cast<unsigned char *>(out), header_.point_record_length);

    // read the next point in
    else
    {
        if (chunk_state_.points_read == laz_.chunk_size || !pdecomperssor_)
        {
            pdecomperssor_ = build_las_decompressor(stream_->cb(),
                    header_.point_format_id, header_.ebCount());
            // reset chunk state
            chunk_state_.current++;
            chunk_state_.points_read = 0;
        }

        pdecomperssor_->decompress(out);
        chunk_state_.points_read++;
    }
}

const io::header& basic_file::header() const
{
    return header_;
}

size_t basic_file::pointCount() const
{
    if (header_.version.major > 1 || header_.version.minor > 3)
        return header14_.point_count_14;
    return header_.point_count;
}

void basic_file::loadHeader()
{
    // Make sure our header is correct
    char magic[4];
    f_->read(magic, sizeof(magic));

    if (std::string(magic, magic + 4) != "LASF")
    {
        throw error("Invalid LAS file. Incorrect magic number.");
    }

    // Read the header in
    f_->seekg(0);
    f_->read((char*)&header_, sizeof(header_));
    // If we're version 4, back up and do it again.
    if (header_.version.minor == 4)
    {
        f_->seekg(0);
        f_->read((char *)&header14_, sizeof(io::header14));
    }
    if (header_.point_format_id & 0x80)
        compressed_ = true;

    // The mins and maxes are in a weird order, fix them
    fixMinMax();

    parseVLRs();

    if (compressed_)
    {
        validateHeader();
        parseChunkTable();
    }

    // set the file pointer to the beginning of data to start reading
    // may have treaded past the EOL, so reset everything before we start reading
    f_->clear();
    uint64_t offset = header_.point_offset;
    if (compressed_)
        offset += sizeof(int64_t);
    f_->seekg(offset);
    stream_->reset();
}

void basic_file::fixMinMax()
{
    double mx, my, mz, nx, ny, nz;

    io::header& h = header_;
    mx = h.minimum.x; nx = h.minimum.y;
    my = h.minimum.z; ny = h.maximum.x;
    mz = h.maximum.y; nz = h.maximum.z;

    h.minimum.x = nx; h.maximum.x = mx;
    h.minimum.y = ny; h.maximum.y = my;
    h.minimum.z = nz; h.maximum.z = mz;
}

void basic_file::parseVLRs()
{
    // move the pointer to the begining of the VLRs
    f_->seekg(header_.header_size);

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
    while (count < header_.vlr_count && f_->good() && !f_->eof())
    {
        f_->read((char*)&vlr_header, sizeof(vlr_header));

        const char *user_id = "laszip encoded";

        if (std::equal(vlr_header.user_id, vlr_header.user_id + 14, user_id) &&
            vlr_header.record_id == 22204)
        {
            laszipFound = true;

            std::unique_ptr<char> buffer(new char[vlr_header.record_length]);
            f_->read(buffer.get(), vlr_header.record_length);
            parseLASZIPVLR(buffer.get());
            break; // no need to keep iterating
        }
        f_->seekg(vlr_header.record_length, std::ios::cur); // jump foward
        count++;
    }

    if (compressed_ && !laszipFound)
        throw error("Couldn't find LASZIP VLR");
}

void basic_file::parseLASZIPVLR(const char *buf)
{
    laz_.fill(buf);

    if (laz_.compressor != 2)
        throw error("LASZIP format unsupported - invalid compressor version.");
}

void basic_file::parseChunkTable()
{
    // Move to the begining of the data
    f_->seekg(header_.point_offset);

    int64_t chunkoffset = 0;
    f_->read((char*)&chunkoffset, sizeof(chunkoffset));
    if (!f_->good())
        throw error("Couldn't read chunk table.");

    if (chunkoffset == -1)
        throw error("Chunk table offset == -1 is not supported at this time");

    // Go to the chunk offset and read in the table
    f_->seekg(chunkoffset);
    if (!f_->good())
        throw error("Error reading chunk table.");

    // Now read in the chunk table
#pragma pack(push, 1)
    struct
    {
        uint32_t version;
        uint32_t chunk_count;
    } chunk_table_header;
#pragma pack(pop)

    f_->read((char *)&chunk_table_header, sizeof(chunk_table_header));
    if (!f_->good())
        throw error("Error reading chunk table.");

    if (chunk_table_header.version != 0)
        throw error("Bad chunk table. Invalid version.");

    // start pushing in chunk table offsets
    chunk_table_offsets_.clear();

    if (laz_.chunk_size == (std::numeric_limits<unsigned int>::max)())
        throw error("Chunk size too large - unsupported.");

    // Allocate enough room for our chunk
    chunk_table_offsets_.resize(chunk_table_header.chunk_count + 1);

    // Add The first one
    chunk_table_offsets_[0] = header_.point_offset + sizeof(uint64_t);

    //ABELL - Use the standard call for this.
    if (chunk_table_header.chunk_count > 1)
    {
        // decode the index out
        InFileStream fstream(*f_);

        InCbStream stream(fstream.cb());
        decoders::arithmetic<InCbStream> decoder(stream);
        decompressors::integer decomp(32, 2);

        // start decoder
        decoder.readInitBytes();
        decomp.init();

        for (size_t i = 1 ; i <= chunk_table_header.chunk_count; i++)
        {
            uint64_t offset = i > 1 ? (int32_t)chunk_table_offsets_[i - 1] : 0;
            chunk_table_offsets_[i] = static_cast<uint64_t>(
                    decomp.decompress(decoder, offset, 1));
        }

        for (size_t i = 1 ; i < chunk_table_offsets_.size() ; i ++)
            chunk_table_offsets_[i] += chunk_table_offsets_[i - 1];
    }
}

void basic_file::validateHeader()
{
    io::header& h = header_;

    int bit_7 = (h.point_format_id >> 7) & 1;
    int bit_6 = (h.point_format_id >> 6) & 1;

    if (bit_7 == 1 && bit_6 == 1)
        throw error("Header bits indicate unsupported old-style compression.");
    if ((bit_7 ^ bit_6) == 0)
        throw error("Header indicates the file is not compressed.");
    h.point_format_id &= 0x3f;
}

mem_file::mem_file(char *buf, size_t count) : sbuf_(buf, count), f_(&sbuf_)
{
    open(f_);
}

generic_file::generic_file(std::istream& in)
{
    open(in);
}

named_file::named_file(const std::string& filename) : f_(filename, std::ios::binary)
{
    open(f_);
}

} // namespace reader

// WRITER

namespace writer
{

basic_file::basic_file() : header_(header14_), chunk_size_(DefaultChunkSize), f_(nullptr)
{}

basic_file::~basic_file()
{}

bool basic_file::compressed() const
{
    return chunk_size_ > 0;
}

void basic_file::open(std::ostream& out, const io::header& h, uint32_t chunk_size)
{
    header_ = h;
    chunk_size_ = chunk_size;
    f_ = &out;

    size_t preludeSize = h.version.minor == 4 ? sizeof(io::header14) : sizeof(io::header);
    if (compressed())
    {
        preludeSize += sizeof(int64_t);  // Chunk table offset.
        laz_vlr vlr(h.point_format_id, h.ebCount(), chunk_size_);
        preludeSize += vlr.size() + vlr.header().size();
    }
    if (h.ebCount())
    {
        eb_vlr vlr(h.ebCount());
        preludeSize += vlr.size() + vlr.header().size();
    }

    std::vector<char> junk(preludeSize);
    f_->write(junk.data(), preludeSize);
    // the first chunk begins at the end of prelude
    stream_.reset(new OutFileStream(out));
}

void basic_file::updateMinMax(const las::point10& p)
{
    double x = p.x * header_.scale.x + header_.offset.x;
    double y = p.y * header_.scale.y + header_.offset.y;
    double z = p.z * header_.scale.z + header_.offset.z;

    header_.minimum.x = (std::min)(x, header_.minimum.x);
    header_.minimum.y = (std::min)(y, header_.minimum.y);
    header_.minimum.z = (std::min)(z, header_.minimum.z);

    header_.maximum.x = (std::max)(x, header_.maximum.x);
    header_.maximum.y = (std::max)(y, header_.maximum.y);
    header_.maximum.z = (std::max)(z, header_.maximum.z);
}

void basic_file::writePoint(const char *p)
{
    if (!compressed())
        stream_->putBytes(reinterpret_cast<const unsigned char *>(p), header_.point_record_length);
    else
    {
        //ABELL - This first bit can go away if we simply always create compressor.
        if (!pcompressor_)
        {
            pcompressor_ = build_las_compressor(stream_->cb(), header_.point_format_id,
                header_.ebCount());
        }
        else if (chunk_state_.points_in_chunk == chunk_size_)
        {
            pcompressor_->done();
            chunk_state_.points_in_chunk = 0;
            std::streamsize offset = f_->tellp();
            chunk_sizes_.push_back(offset - chunk_state_.last_chunk_write_offset);
            chunk_state_.last_chunk_write_offset = offset;
            pcompressor_ = build_las_compressor(stream_->cb(), header_.point_format_id,
                header_.ebCount());
        }

        // now write the point
        pcompressor_->compress(p);
    }
    chunk_state_.total_written++;
    chunk_state_.points_in_chunk++;
    updateMinMax(*(reinterpret_cast<const las::point10*>(p)));
}

void basic_file::close()
{
    if (compressed())
    {
        pcompressor_->done();

        // Note down the size of the offset of this last chunk
        chunk_sizes_.push_back((std::streamsize)f_->tellp() - chunk_state_.last_chunk_write_offset);
    }

    writeHeader();
    if (compressed())
        writeChunkTable();
}

void basic_file::writeHeader()
{
    laz_vlr lazVlr(header_.point_format_id, header_.ebCount(), chunk_size_);
    eb_vlr ebVlr(header_.ebCount());

    // point_format_id and point_record_length  are set on open().
    header_.header_size = (header_.version.minor == 4 ? sizeof(io::header14) : sizeof(io::header));
    header_.point_offset = header_.header_size;
    header_.vlr_count = 0;
    if (compressed())
    {
       header_.point_offset += lazVlr.size() + lazVlr.header().size();
       header_.vlr_count++;
       header_.point_format_id |= (1 << 7);
    }
    if (header_.ebCount())
    {
        header_.point_offset += ebVlr.size() + ebVlr.header().size();
        header_.vlr_count++;
    }

    header_.point_count = static_cast<unsigned int>(chunk_state_.total_written);

    //HUH?
    // make sure we re-arrange mins and maxs for writing
    double mx, my, mz, nx, ny, nz;
    nx = header_.minimum.x; mx = header_.maximum.x;
    ny = header_.minimum.y; my = header_.maximum.y;
    nz = header_.minimum.z; mz = header_.maximum.z;

    header_.minimum.x = mx; header_.minimum.y = nx;
    header_.minimum.z = my; header_.maximum.x = ny;
    header_.maximum.y = mz; header_.maximum.z = nz;

    if (header_.version.minor == 4)
    {
        header14_.point_count_14 = header_.point_count;
        // Set the WKT bit.
        header_.global_encoding |= (1 << 4);
    }

    f_->seekp(0);
    f_->write(reinterpret_cast<char*>(&header_), header_.header_size);

    if (compressed())
    {
        // Write the VLR.
        vlr::vlr_header h = lazVlr.header();
        f_->write(reinterpret_cast<char *>(&h), sizeof(h));

        std::vector<char> vlrbuf = lazVlr.data();
        f_->write(vlrbuf.data(), vlrbuf.size());
    }
    if (header_.ebCount())
    {
        vlr::vlr_header h = ebVlr.header();
        f_->write(reinterpret_cast<char *>(&h), sizeof(h));

        std::vector<char> vlrbuf = ebVlr.data();
        f_->write(vlrbuf.data(), vlrbuf.size());
    }
}

void basic_file::writeChunkTable()
{
    // move to the end of the file to start emitting our compresed table
    f_->seekp(0, std::ios::end);

    // take note of where we're writing the chunk table, we need this later
    int64_t chunk_table_offset = static_cast<int64_t>(f_->tellp());

    // write out the chunk table header (version and total chunks)
#pragma pack(push, 1)
    struct
    {
        unsigned int version,
        chunks_count;
    } chunk_table_header = { 0, static_cast<unsigned int>(chunk_sizes_.size()) };
#pragma pack(pop)

    f_->write(reinterpret_cast<char*>(&chunk_table_header), sizeof(chunk_table_header));

    // Now compress and write the chunk table
    OutFileStream w(*f_);
    OutCbStream outStream(w.cb());

    encoders::arithmetic<OutCbStream> encoder(outStream);
    compressors::integer comp(32, 2);

    comp.init();

    for (size_t i = 0 ; i < chunk_sizes_.size() ; i ++)
    {
        comp.compress(encoder, i ? static_cast<int>(chunk_sizes_[i-1]) : 0,
                static_cast<int>(chunk_sizes_[i]), 1);
    }
    encoder.done();

    // go back to where we're supposed to write chunk table offset
    f_->seekp(header_.point_offset);
    f_->write(reinterpret_cast<char*>(&chunk_table_offset), sizeof(chunk_table_offset));
}

named_file::config::config() : scale(1.0, 1.0, 1.0), offset(0.0, 0.0, 0.0),
    chunk_size(DefaultChunkSize), pdrf(0), minor_version(3), extra_bytes(0)
{}

named_file::config::config(const io::vector3& s, const io::vector3& o, unsigned int cs) :
    scale(s), offset(o), chunk_size(cs), pdrf(0), minor_version(3), extra_bytes(0)
{}

named_file::config::config(const io::header& h) : scale(h.scale.x, h.scale.y, h.scale.z),
    offset(h.offset.x, h.offset.y, h.offset.z), chunk_size(DefaultChunkSize),
    pdrf(h.point_format_id), extra_bytes(h.ebCount())
{}

io::header named_file::config::to_header() const
{
    io::header h;

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

named_file::named_file(const std::string& filename, const named_file::config& c)
{
    io::header h = c.to_header();

    // open the file and move to offset of data, we'll write
    // headers and all other things on file close
    f_.open(filename, std::ios::binary | std::ios::trunc);
    if (!f_.good())
        throw error("Couldn't open '" + filename + "' for writing.");
    open(f_, h, c.chunk_size);
}

void named_file::close()
{
    basic_file::close();
    if (f_.is_open())
        f_.close();
}

} // namespace writer
} // namespace lazperf

