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

#pragma once

#include <cstdint>
#include <fstream>

#include "las.hpp"
#include "streams.hpp"
#include "vlr.hpp"
#include "charbuf.hpp"

#pragma once

namespace lazperf
{
namespace io
{
const uint32_t DefaultChunkSize = 50000;

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
    char magic[4] { 'L', 'A', 'S', 'F' };
    uint16_t file_source_id {};
    uint16_t global_encoding {};
    char guid[16] {};

    struct {
        uint8_t major {1};
        uint8_t minor {3};
    } version;

    char system_identifier[32] {};
    char generating_software[32] {};

    struct {
        uint16_t day {};
        uint16_t year {};
    } creation;

    uint16_t header_size {};
    uint32_t point_offset {};
    uint32_t vlr_count {};

    uint8_t point_format_id {};
    uint16_t point_record_length {};

    uint32_t point_count {};
    uint32_t points_by_return[5] {};

    vector3 scale;
    vector3 offset;
    vector3 minimum;
    vector3 maximum;

    int ebCount() const;
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
} // namespace io

#define FRIEND_TEST(test_case_name, test_name) \
    friend class test_case_name##_##test_name##_Test

namespace reader
{

class basic_file
{
    FRIEND_TEST(io_tests, parses_laszip_vlr_correctly);
protected:
    basic_file();
    ~basic_file();

    void open(std::istream& in);

public:
    size_t pointCount() const;
    const io::header& header() const;
    void readPoint(char *out);

private:
    void loadHeader();
    void fixMinMax();
    void parseVLRs();
    void parseLASZIPVLR(const char *buf);
    void parseChunkTable();
    void validateHeader();

    // The file object is not copyable or copy constructible
    basic_file(const basic_file&) = delete;
    basic_file& operator = (const basic_file&) = delete;

    std::istream *f_;
    std::unique_ptr<InFileStream> stream_;
    io::header& header_;
    io::header14 header14_;
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

class mem_file : public basic_file
{
public:
    mem_file(char *buf, size_t count);

private:
    charbuf sbuf_;
    std::istream f_;
};

class generic_file : public basic_file
{
public:
    generic_file(std::istream& in);
};

class named_file : public basic_file
{
public:
    named_file(const std::string& filename);

private:
    std::ifstream f_;
};

} // namespace reader


namespace writer
{

class basic_file
{
protected:
    basic_file();
    virtual ~basic_file();

public:
    void open(std::ostream& out, const io::header& h, uint32_t chunk_size);
    void writePoint(const char *p);
    void close();
    virtual bool compressed() const;

private:
    void updateMinMax(const las::point10& p);
    void writeHeader();
    void writeChunks();
    void writeChunkTable();

    las_compressor::ptr pcompressor_;
    io::header& header_;
    io::header14 header14_;
    unsigned int chunk_size_;
    std::ostream *f_;
    std::unique_ptr<OutFileStream> stream_;

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

class named_file : public basic_file
{
public:
    struct config
    {
    public:
        io::vector3 scale;
        io::vector3 offset;
        unsigned int chunk_size;
        int pdrf;
        int minor_version;
        int extra_bytes;

        explicit config();
        config(const io::vector3& scale, const io::vector3& offset,
            unsigned int chunksize = io::DefaultChunkSize);
        config(const io::header& header);

        io::header to_header() const;
    };

    named_file(const std::string& filename, const config& c);
    void close();

private:
    std::ofstream f_;
};

} // namespace writer
} // namespace lazperf

