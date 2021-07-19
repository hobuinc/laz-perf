/*
  COPYRIGHT:

    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#pragma once

// NOTE: This file exists to facilitate testing of private code.

#include "io.hpp"
#include "las.hpp"
#include "charbuf.hpp"
#include "streams.hpp"
#include "vlr.hpp"

namespace lazperf
{
namespace reader
{

struct basic_file::Private
{
    Private() : header(header14), compressed(false), current_chunk(nullptr)
    {}

    void open(std::istream& f);
    void readPoint(char *out);
    void loadHeader();
    uint64_t pointCount() const;
    void fixMinMax();
    void parseVLRs();
    void parseLASZIPVLR(const char *);
    void parseChunkTable();
    void validateHeader();

    std::istream *f;
    std::unique_ptr<InFileStream> stream;
    io::header& header;
    io::header14 header14;
    bool compressed;
    las_decompressor::ptr pdecompressor;
    laz_vlr laz;
    struct Chunk
    {
        uint64_t count;
        uint64_t offset;
    };
    Chunk *current_chunk;
    uint32_t chunk_point_num;
    std::vector<Chunk> chunks;
};

struct mem_file::Private
{
    Private(char *buf, size_t count) : sbuf(buf, count), f(&sbuf)
    {}

    charbuf sbuf;
    std::istream f;
};

struct named_file::Private
{
    Private(const std::string& filename) : f(filename, std::ios::binary)
    {}

    std::ifstream f;
};

} // namespace reader

namespace writer
{

struct basic_file::Private
{
    Private() : chunk_size(io::DefaultChunkSize), header(header14), f(nullptr)
    {}

    void close();
    uint64_t chunk();
    uint64_t firstChunkOffset() const;
    bool compressed() const;
    void open(std::ostream& out, const io::header& h, uint32_t chunk_size);
    void writePoint(const char *p);
    void updateMinMax(const las::point10& p);
    void writeHeader();
    void writeChunks();
    void writeChunkTable();

    struct Chunk
    {
        uint64_t count;
        uint64_t offset;
    };
    uint64_t chunk_offset;
    uint32_t chunk_point_num;
    uint32_t chunk_size;
    std::vector<Chunk> chunks;
    las_compressor::ptr pcompressor;
    io::header& header;
    io::header14 header14;
    std::ostream *f;
    std::unique_ptr<OutFileStream> stream;
};

struct named_file::Private
{
    using Base = basic_file::Private;

    Private(Base *b) : base(b)
    {}
    
    void open(const std::string& filename, const named_file::config& c);

    Base *base;
    std::ofstream f;
};

} // namespace writer
} // namespace lazperf

