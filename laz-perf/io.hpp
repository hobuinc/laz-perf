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

#include <fstream>
#include <functional>
#include <limits>
#include <string.h>
#include <mutex>

#include "formats.hpp"
#include "excepts.hpp"
#include "factory.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "laz_vlr.hpp"
#include "util.hpp"
#include "portable_endian.hpp"

namespace laszip
{
namespace io
{
static const int DefaultChunkSize = 50000;

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

// cache line
#define BUF_SIZE (1 << 20)

template<typename StreamType>
struct __ifstream_wrapper
{
    __ifstream_wrapper(StreamType& f) : f_(f), offset(0), have(0),
        buf_((char*)utils::aligned_malloc(BUF_SIZE))
    {}

    ~__ifstream_wrapper()
    {
        utils::aligned_free(buf_);
    }

    __ifstream_wrapper(const __ifstream_wrapper<StreamType>&) = delete;
    __ifstream_wrapper& operator = (const __ifstream_wrapper<StreamType>&) = delete;

    inline void fillit_()
    {
        offset = 0;
        f_.read(buf_, BUF_SIZE);
        have = f_.gcount();

        // this is an exception since we shouldn't be hitting eof
        if (have == 0)
            throw error("Unexpected end of file.");
    }

    inline void reset()
    {
        offset = have = 0; // when a file is seeked, reset this
    }

    inline unsigned char getByte()
    {
        if (offset >= have)
            fillit_();
        return static_cast<unsigned char>(buf_[offset++]);
    }

    inline void getBytes(unsigned char *buf, size_t request)
    {
        // Use what's left in the buffer, if anything.
        size_t fetchable = (std::min)((size_t)(have - offset), request);
        std::copy(buf_ + offset, buf_ + offset + fetchable, buf);
        offset += fetchable;
        request -= fetchable;

        // If we couldn't fetch everything requested, fill buffer
        // and go again.  We assume fillit_() satisfies any request.
        if (request)
        {
            fillit_();
            std::copy(buf_ + offset, buf_ + offset + request, buf + fetchable);
            offset += request;
        }
    }

    StreamType& f_;
    std::streamsize offset, have;
    char *buf_;
};

template<typename StreamType>
struct __ofstream_wrapper
{
    __ofstream_wrapper(StreamType& f) : f_(f)
    {}

    void putBytes(const unsigned char *b, size_t len)
    {
        f_.write(reinterpret_cast<const char*>(b), len);
    }

    void putByte(unsigned char b)
    {
        f_.put((char)b);
    }

    __ofstream_wrapper(const __ofstream_wrapper&) = delete;
    __ofstream_wrapper& operator = (const __ofstream_wrapper&) = delete;

    StreamType& f_;
};


namespace reader
{

template <typename StreamType>
class basic_file
{
    typedef std::function<void (header&)> validator_type;
    typedef __ifstream_wrapper<StreamType> LazPerfStream;

public:
    basic_file(StreamType& st) : f_(st), stream_(f_), header_(header14_), compressed_(false)
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

     const factory::record_schema& get_schema() const
     {
         return schema_;
     }

     void readPoint(char *out)
     {
         if (!compressed_)
             stream_.getBytes(reinterpret_cast<unsigned char *>(out), schema_.size_in_bytes());

         // read the next point in
         else
         {
            if (chunk_state_.points_read == laz_.chunk_size || !pdecomperssor_)
            {
                pdecomperssor_ = factory::build_las_decompressor(stream_, schema_);

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
            throw error("Invalid LAS file. Incorrect magic number.");

        // Read the header in
        f_.seekg(0);
        f_.read((char*)&header_, sizeof(header_));
        if (header_.version.minor == 4)
        {
            f_.seekg(0);
            f_.read((char *)&header14_, sizeof(header14));
        }
        if (header_.point_format_id & 0x80)
            compressed_ = true;

        // The mins and maxes are in a weird order, fix them
        _fixMinMax(header_);

        if (compressed_)
        {
            // Make sure everything is valid with the header, note that validators are allowed
            // to manipulate the header, since certain validators depend on a header's
            // original state to determine what its final stage is going to be
            for (auto f : _validators())
                f(header_);

            // things look fine, move on with VLR extraction
            _parseLASZIP();

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

    void _parseLASZIP()
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

        if (!laszipFound)
            throw error("Couldn't find LASZIP VLR");
        schema_ = laz_vlr::to_schema(laz_, header_.point_record_length);
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
        struct
        {
            unsigned int version,
            chunk_count;
        } chunk_table_header;

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
            LazPerfStream w(f_);

            decoders::arithmetic<LazPerfStream> decoder(w);
            decompressors::integer decomp(32, 2);

            // start decoder
            decoder.readInitBytes();
            decomp.init();

            for (size_t i = 1 ; i <= chunk_table_header.chunk_count ; i ++)
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
        static std::mutex lock;

        // To remain thread safe we need to make sure we have appropriate guards here
        if (v.empty())
        {
            lock.lock();
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
            lock.unlock();
        }
        return v;
    }

    // The file object is not copyable or copy constructible
    basic_file(const basic_file<StreamType>&) = delete;
    basic_file<StreamType>& operator = (const basic_file<StreamType>&) = delete;

    StreamType& f_;
    LazPerfStream stream_;
    header& header_;
    header14 header14_;
    laz_vlr laz_;
    std::vector<uint64_t> chunk_table_offsets_;
    bool compressed_;

    // the schema of this file, the LAZ items converted into factory recognizable description,
    factory::record_schema schema_;

    formats::las_decompressor::ptr pdecomperssor_;

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

typedef basic_file<std::ifstream> file;
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
    file() : wrapper_(f_), header_(header14_)
    {}

    file(const std::string& filename, const factory::record_schema& s, const config& config) :
        wrapper_(f_), schema_(s), header_(header14_)
    {
        open(filename, s, config);
    }

    void open(const std::string& filename, const factory::record_schema& s, const config& c)
    {
        chunk_size_ = c.chunk_size;
        compressed_ = c.compressed;
        minor_version_ = c.minor_version;

        // open the file and move to offset of data, we'll write
        // headers and all other things on file close
        f_.open(filename, std::ios::binary | std::ios::trunc);
        if (!f_.good())
            throw error("Couldn't open '" + filename + "' for writing.");

        schema_ = s;
        header_ = c.to_header();
        chunk_size_ = c.chunk_size;

        // write junk to our prelude, we'll overwrite this with
        // awesome data later
        size_t preludeSize = c.minor_version == 4 ? sizeof(header14) : sizeof(header);
        if (compressed_)
            preludeSize +=
                54 + // size of one vlr header
                (34 + s.records_.size() * 6) + // the LAZ vlr size
                sizeof(int64_t); // chunk table offset

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
            wrapper_.putBytes(reinterpret_cast<const unsigned char *>(p), schema_.size_in_bytes());
        }
        else
        {
            //ABELL - This first bit can go away if we simply always create compressor and
            //  decompressor.
            if (!pcompressor_)
            {
                pcompressor_ = factory::build_las_compressor(wrapper_, schema_);
            }
            else if (chunk_state_.points_in_chunk == chunk_size_)
            {
                pcompressor_->done();
                chunk_state_.points_in_chunk = 0;
                std::streamsize offset = f_.tellp();
                chunk_sizes_.push_back(offset - chunk_state_.last_chunk_write_offset);
                chunk_state_.last_chunk_write_offset = offset;
                pcompressor_ = factory::build_las_compressor(wrapper_, schema_);
            }

            // now write the point
            pcompressor_->compress(p);
        }
        chunk_state_.total_written++;
        chunk_state_.points_in_chunk++;
        _update_min_max(*(reinterpret_cast<const formats::las::point10*>(p)));
    }

    void close()
    {
        _flush();

        if (f_.is_open())
            f_.close();
    }

private:
    void _update_min_max(const formats::las::point10& p)
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

        header_.point_format_id = schema_.format();

        header_.header_size = minor_version_ == 4 ? sizeof(header14) : sizeof(header);
        header_.point_offset = header_.header_size;
        if (compressed_)
        {
            // 54 is the size of one vlr header
            header_.point_offset += 54 +
                (34 + static_cast<unsigned int>(schema_.records_.size()) * 6);
            header_.vlr_count = 1;
            header_.point_format_id |= (1 << 7);
        }
        else
            header_.vlr_count = 0;

        header_.point_record_length = static_cast<unsigned short>(schema_.size_in_bytes());
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
            laz_vlr vlr = laz_vlr::from_schema(schema_, chunk_size_);

            vlr::vlr_header h = vlr.header();
            f_.write(reinterpret_cast<char *>(&h), sizeof(h));

            std::unique_ptr<char> vlrbuf(new char[vlr.size()]);
            vlr.extract(vlrbuf.get());
            f_.write(vlrbuf.get(), vlr.size());
            _writeChunks();
        }
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
        __ofstream_wrapper<std::ofstream> w(f_);

        encoders::arithmetic<__ofstream_wrapper<std::ofstream> > encoder(w);
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
    __ofstream_wrapper<std::ofstream> wrapper_;

    formats::las_compressor::ptr pcompressor_;

    factory::record_schema schema_;
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
} // namespace laszip

#endif // __io_hpp__
