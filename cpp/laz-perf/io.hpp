/*
===============================================================================

  FILE:  io.hpp

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

#ifndef __io_hpp__
#define __io_hpp__

#include <iostream>
#include <fstream>
#include <functional>
#include <limits>
#include <string.h>

#include "compressor.hpp"
#include "decompressor.hpp"
#include "excepts.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "las.hpp"
#include "laz_vlr.hpp"
#include "eb_vlr.hpp"
#include "utils.hpp"
#include "streams.hpp"
#include "portable_endian.hpp"

namespace lazperf
{
namespace io
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


#pragma pack(push, 1)
struct vector3
{
    vector3() : x(0), y(0), z(0)
    {}

    vector3(double x, double y, double z) : x(x), y(y), z(z)
    {}

    double x;
    double y;
    double z;
};

struct header
{
    char magic[4];
    uint16_t file_source_id;
    uint16_t global_encoding;
    char guid[16];

    struct {
        uint8_t major;
        uint8_t minor;
    } version;

    char system_identifier[32];
    char generating_software[32];

    struct {
        uint16_t day;
        uint16_t year;
    } creation;

    uint16_t header_size;
    uint32_t point_offset;
    uint32_t vlr_count;

    uint8_t point_format_id;
    uint16_t point_record_length;

    uint32_t point_count;
    uint32_t points_by_return[5];

    vector3 scale;
    vector3 offset;
    vector3 minimum;
    vector3 maximum;

    int ebCount()
    {
        int baseSize = baseCount(point_format_id);
        return (baseSize ? point_record_length - baseSize : 0);
    }
};

struct header14 : public header
{
    uint64_t wave_offset {0};
    uint64_t evlr_offset {0};
    uint32_t elvr_count {0};
    uint64_t point_count_14 {0};
    uint64_t points_by_return_14[15] {};
};
#pragma pack(pop)

namespace reader
{

class basic_file
{
    typedef std::function<void (header&)> validator_type;

public:
    basic_file(std::istream& st) : f_(st), stream_(f_), header_(header14_), compressed_(false)
    {
        _open();
    }

    ~basic_file()
    {}

     const header& get_header() const
     {
         return header_;
     }

     const laz_vlr& get_laz_vlr() const
     {
         return laz_;
     }

    std::vector<uint64_t> chunkOffsets() const
    {
        return chunk_table_offsets_;
    }

    uint32_t chunkSize() const
    {
        return laz_.chunk_size;
    }

    int64_t numPoints() const
    {
        return laz_.num_points;
    }

    uint32_t pointSize()
    {
        return header_.point_record_length;
    }

    void readPoint(char *out)
    {
        if (!compressed_)
            stream_.cb()(reinterpret_cast<unsigned char *>(out), header_.point_record_length);

        // read the next point in
        else
        {
            if (chunk_state_.points_read == laz_.chunk_size || !pdecomperssor_)
            {
                pdecomperssor_ = build_las_decompressor(stream_.cb(),
                    header_.point_format_id, header_.ebCount());
                // reset chunk state
                chunk_state_.current++;
                chunk_state_.points_read = 0;
            }

            pdecomperssor_->decompress(out);
            chunk_state_.points_read++;
        }
    }

private:
    void _open()
    {
        // Make sure our header is correct
        char magic[4];
        f_.read(magic, sizeof(magic));

        if (std::string(magic, magic + 4) != "LASF")
        {
            throw error("Invalid LAS file. Incorrect magic number.");
        }

        // Read the header in
        f_.seekg(0);
        f_.read((char*)&header_, sizeof(header_));
        // If we're version 4, back up and do it again.
        if (header_.version.minor == 4)
        {
            f_.seekg(0);
            f_.read((char *)&header14_, sizeof(header14));
        }
        if (header_.point_format_id & 0x80)
            compressed_ = true;

        // The mins and maxes are in a weird order, fix them
        _fixMinMax(header_);

        parseVLRs();

        if (compressed_)
        {
            // Make sure everything is valid with the header, note that validators are allowed
            // to manipulate the header, since certain validators depend on a header's
            // original state to determine what its final stage is going to be
            for (auto f : _validators())
                f(header_);

            // parse the chunk table offset
            _parseChunkTable();
        }

        // set the file pointer to the beginning of data to start reading
        // may have treaded past the EOL, so reset everything before we start reading
        f_.clear();
        uint64_t offset = header_.point_offset;
        if (compressed_)
            offset += sizeof(int64_t);
        f_.seekg(offset);
        stream_.reset();
    }

    void _fixMinMax(header& h)
    {
        double mx, my, mz, nx, ny, nz;

        mx = h.minimum.x; nx = h.minimum.y;
        my = h.minimum.z; ny = h.maximum.x;
        mz = h.maximum.y; nz = h.maximum.z;

        h.minimum.x = nx; h.maximum.x = mx;
        h.minimum.y = ny; h.maximum.y = my;
        h.minimum.z = nz; h.maximum.z = mz;
    }

    void parseVLRs()
    {
        // move the pointer to the begining of the VLRs
        f_.seekg(header_.header_size);

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
        while (count < header_.vlr_count && f_.good() && !f_.eof())
        {
            f_.read((char*)&vlr_header, sizeof(vlr_header));

            const char *user_id = "laszip encoded";

            if (std::equal(vlr_header.user_id, vlr_header.user_id + 14, user_id) &&
                vlr_header.record_id == 22204)
            {
                laszipFound = true;

                std::unique_ptr<char> buffer(new char[vlr_header.record_length]);
                f_.read(buffer.get(), vlr_header.record_length);
                _parseLASZIPVLR(buffer.get());
                break; // no need to keep iterating
            }
            f_.seekg(vlr_header.record_length, std::ios::cur); // jump foward
            count++;
        }

        if (compressed_ && !laszipFound)
            throw error("Couldn't find LASZIP VLR");
    }

    void binPrint(const char *buf, int len)
    {
        for (int i = 0 ; i < len ; i ++)
        {
            char b[256];
            sprintf(b, "%02X", buf[i] & 0xFF);
            std::cout << b << " ";
        }

        std::cout << std::endl;
    }

    void _parseLASZIPVLR(const char *buf)
    {
        laz_.fill(buf);

        if (laz_.compressor != 2)
            throw error("LASZIP format unsupported - invalid compressor version.");
    }

    void _parseChunkTable()
    {
        // Move to the begining of the data
        f_.seekg(header_.point_offset);

        int64_t chunkoffset = 0;
        f_.read((char*)&chunkoffset, sizeof(chunkoffset));
        if (!f_.good())
            throw error("Couldn't read chunk table.");

        if (chunkoffset == -1)
            throw error("Chunk table offset == -1 is not supported at this time");

        // Go to the chunk offset and read in the table
        f_.seekg(chunkoffset);
        if (!f_.good())
            throw error("Error reading chunk table.");

        // Now read in the chunk table
#pragma pack(push, 1)
        struct
        {
            uint32_t version;
            uint32_t chunk_count;
        } chunk_table_header;
#pragma pack(pop)

        f_.read((char *)&chunk_table_header, sizeof(chunk_table_header));
        if (!f_.good())
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

        if (chunk_table_header.chunk_count > 1)
        {
            // decode the index out
            InFileStream fstream(f_);

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

    static const std::vector<validator_type>& _validators()
    {
        static std::vector<validator_type> v; // static collection of validators

        // To remain thread safe we need to make sure we have appropriate guards here
        if (v.empty())
        {
            // Double check here if we're still empty, the first empty just makes sure
            // we have a quick way out where validators are already filled up (for all calls
            // except the first one), for two threads competing to fill out the validators
            // only one of the will get here first, and the second one will bail if the v
            // is not empty, and hence the double check
            if (v.empty())
            {
                // Make sure that the header indicates that file is compressed
                v.push_back(
                    [](header& h)
                    {
                        int bit_7 = (h.point_format_id >> 7) & 1;
                        int bit_6 = (h.point_format_id >> 6) & 1;

                        if (bit_7 == 1 && bit_6 == 1)
                            throw error("Header bits indicate unsupported old-style compression.");
                        if ((bit_7 ^ bit_6) == 0)
                            throw error("Header indicates the file is not compressed.");
                        h.point_format_id &= 0x3f;
                    }
                );
            }
        }
        return v;
    }

    // The file object is not copyable or copy constructible
    basic_file(const basic_file&) = delete;
    basic_file& operator = (const basic_file&) = delete;

    std::istream& f_;
    InFileStream stream_;
    header& header_;
    header14 header14_;
    laz_vlr laz_;
    std::vector<uint64_t> chunk_table_offsets_;
    bool compressed_;

    las_decompressor::ptr pdecomperssor_;

    // Establish our current state as we iterate through the file
    struct __chunk_state
    {
        int64_t current;
        int64_t points_read;
        int64_t current_index;

        __chunk_state() : current(0u), points_read(0u), current_index(-1)
        {}
    } chunk_state_;
};

} // namespace reader

namespace writer
{

// An object to encapsulate what gets passed to
struct config
{
    vector3 scale;
    vector3 offset;
    unsigned int chunk_size;
    bool compressed;
    int minor_version;
    int extra_bytes;

    explicit config() :
        scale(1.0, 1.0, 1.0), offset(0.0, 0.0, 0.0), chunk_size(DefaultChunkSize), compressed(true)
    {}

    config(const vector3& s, const vector3& o, unsigned int cs = DefaultChunkSize) :
        scale(s), offset(o), chunk_size(cs), compressed(true)
    {}

    config(const header& h) : scale(h.scale.x, h.scale.y, h.scale.z),
        offset(h.offset.x, h.offset.y, h.offset.z), chunk_size(DefaultChunkSize), compressed(true)
    {}

    header to_header() const
    {
        header h;

        memset(&h, 0, sizeof(h)); // clear out header
        h.minimum = { (std::numeric_limits<double>::max)(), (std::numeric_limits<double>::max)(),
            (std::numeric_limits<double>::max)() };
        h.maximum = { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest()};

        h.offset.x = offset.x;
        h.offset.y = offset.y;
        h.offset.z = offset.z;

        h.scale.x = scale.x;
        h.scale.y = scale.y;
        h.scale.z = scale.z;

        return h;
    }
};

class file
{
public:
    file() : stream_(f_), header_(header14_)
    {}

    file(const std::string& filename, int format, int eb_count, const config& config) :
        stream_(f_), header_(header14_)
    {
        open(filename, format, eb_count, config);
    }

    void open(const std::string& filename, int format, int eb_count, const config& c)
    {
        chunk_size_ = c.chunk_size;
        compressed_ = c.compressed;
        minor_version_ = c.minor_version;


        // open the file and move to offset of data, we'll write
        // headers and all other things on file close
        f_.open(filename, std::ios::binary | std::ios::trunc);
        if (!f_.good())
            throw error("Couldn't open '" + filename + "' for writing.");

        header_ = c.to_header();
        header_.point_format_id = format;
        header_.point_record_length = baseCount(format) + eb_count;
        chunk_size_ = c.chunk_size;

        size_t preludeSize = c.minor_version == 4 ? sizeof(header14) : sizeof(header);
        if (compressed_)
        {
            preludeSize += sizeof(int64_t);  // Chunk table offset.
            laz_vlr vlr(format, eb_count, DefaultChunkSize);
            preludeSize += vlr.size() + vlr.header().size();
        }
        if (eb_count)
        {
            eb_vlr vlr(eb_count);
            preludeSize += vlr.size() + vlr.header().size();
        }

        char *junk = new char[preludeSize];
        std::fill(junk, junk + preludeSize, 0);
        f_.write(junk, preludeSize);
        delete [] junk;
        // the first chunk begins at the end of prelude
    }

    void writePoint(const char *p)
    {
        if (!compressed_)
        {
            stream_.putBytes(reinterpret_cast<const unsigned char *>(p),
                header_.point_record_length);
        }
        else
        {
            //ABELL - This first bit can go away if we simply always create compressor and
            //  decompressor.
            if (!pcompressor_)
            {
                pcompressor_ = build_las_compressor(stream_.cb(), header_.point_format_id,
                    header_.ebCount());
            }
            else if (chunk_state_.points_in_chunk == chunk_size_)
            {
                pcompressor_->done();
                chunk_state_.points_in_chunk = 0;
                std::streamsize offset = f_.tellp();
                chunk_sizes_.push_back(offset - chunk_state_.last_chunk_write_offset);
                chunk_state_.last_chunk_write_offset = offset;
                pcompressor_ = build_las_compressor(stream_.cb(), header_.point_format_id,
                    header_.ebCount());
            }

            // now write the point
            pcompressor_->compress(p);
        }
        chunk_state_.total_written++;
        chunk_state_.points_in_chunk++;
        _update_min_max(*(reinterpret_cast<const las::point10*>(p)));
    }

    void close()
    {
        _flush();
        if (f_.is_open())
            f_.close();
    }

private:
    void _update_min_max(const las::point10& p)
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

    void _flush()
    {
        laz_vlr lazVlr(header_.point_format_id, header_.ebCount(), chunk_size_);
        eb_vlr ebVlr(header_.ebCount());

        if (compressed_)
        {
            // flush out the encoder
            pcompressor_->done();

            // Note down the size of the offset of this last chunk
            chunk_sizes_.push_back((std::streamsize)f_.tellp() -
                chunk_state_.last_chunk_write_offset);
        }

        // Time to write our header
        // Fill up things not filled up by our header
        header_.magic[0] = 'L'; header_.magic[1] = 'A';
        header_.magic[2] = 'S'; header_.magic[3] = 'F';

        header_.version.major = 1;
        header_.version.minor = minor_version_;

        // point_format_id and point_record_length  are set on open().
        header_.header_size = minor_version_ == 4 ? sizeof(header14) : sizeof(header);
        header_.point_offset = header_.header_size;
        header_.vlr_count = 0;
        if (compressed_)
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

        // make sure we re-arrange mins and maxs for writing
        double mx, my, mz, nx, ny, nz;
        nx = header_.minimum.x; mx = header_.maximum.x;
        ny = header_.minimum.y; my = header_.maximum.y;
        nz = header_.minimum.z; mz = header_.maximum.z;

        header_.minimum.x = mx; header_.minimum.y = nx;
        header_.minimum.z = my; header_.maximum.x = ny;
        header_.maximum.y = mz; header_.maximum.z = nz;

        if (minor_version_ == 4)
        {
            header14_.point_count_14 = header_.point_count;
            // Set the WKT bit.
            header_.global_encoding |= (1 << 4);
        }

        f_.seekp(0);
        f_.write(reinterpret_cast<char*>(&header_), header_.header_size);

        if (compressed_)
        {
            // Write the VLR.
            vlr::vlr_header h = lazVlr.header();
            f_.write(reinterpret_cast<char *>(&h), sizeof(h));

            std::vector<uint8_t> vlrbuf = lazVlr.data();
            f_.write((const char *)vlrbuf.data(), vlrbuf.size());
        }
        if (header_.ebCount())
        {
            vlr::vlr_header h = ebVlr.header();
            f_.write(reinterpret_cast<char *>(&h), sizeof(h));

            std::vector<uint8_t> vlrbuf = ebVlr.data();
            f_.write((const char *)vlrbuf.data(), vlrbuf.size());
        }

        if (compressed_)
            _writeChunks();
    }

    void _writeChunks()
    {
        // move to the end of the file to start emitting our compresed table
        f_.seekp(0, std::ios::end);

        // take note of where we're writing the chunk table, we need this later
        int64_t chunk_table_offset = static_cast<int64_t>(f_.tellp());

        // write out the chunk table header (version and total chunks)
#pragma pack(push, 1)
        struct
        {
            unsigned int version,
            chunks_count;
        } chunk_table_header = { 0, static_cast<unsigned int>(chunk_sizes_.size()) };
#pragma pack(pop)

        f_.write(reinterpret_cast<char*>(&chunk_table_header), sizeof(chunk_table_header));

        // Now compress and write the chunk table
        OutFileStream w(f_);
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
        f_.seekp(header_.point_offset);
        f_.write(reinterpret_cast<char*>(&chunk_table_offset), sizeof(chunk_table_offset));
    }

    std::ofstream f_;
    OutFileStream stream_;

    las_compressor::ptr pcompressor_;

    header& header_;
    header14 header14_;
    unsigned int chunk_size_;
    bool compressed_;
    int minor_version_;

    struct __chunk_state
    {
        int64_t total_written; // total points written
        int64_t current_chunk_index; //  the current chunk index we're compressing
        unsigned int points_in_chunk;
        std::streamsize last_chunk_write_offset;

        __chunk_state() : total_written(0), current_chunk_index(-1),
            points_in_chunk(0), last_chunk_write_offset(0)
        {}
    } chunk_state_;

    std::vector<int64_t> chunk_sizes_; // all the places where chunks begin
};

} // namespace writer
} // namespace io
} // namespace lazperf

#endif // __io_hpp__
