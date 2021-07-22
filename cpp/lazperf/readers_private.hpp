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

#include "readers.hpp"
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
    Private() : header(header14), header13(header14), compressed(false), current_chunk(nullptr)
    {}

    void open(std::istream& f);
    uint64_t firstChunkOffset() const;
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
    io::base_header& header;
    io::header13& header13;
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
} // namespace lazperf

