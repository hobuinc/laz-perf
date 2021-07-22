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

#include "writers.hpp"
#include "las.hpp"
#include "streams.hpp"
#include "vlr.hpp"
#include "io_private.hpp"

namespace lazperf
{
namespace writer
{

struct basic_file::Private
{
    Private() : chunk_size(io::DefaultChunkSize), header(header14), header13(header14),
        f(nullptr)
    {}

    void close();
    uint64_t chunk();
    uint64_t firstChunkOffset() const;
    bool compressed() const;
    void open(std::ostream& out, const io::base_header& h, uint32_t chunk_size);
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
    uint64_t prev_chunk_offset;
    uint32_t chunk_point_num;
    uint32_t chunk_size;
    std::vector<Chunk> chunks;
    las_compressor::ptr pcompressor;
    io::base_header& header;
    io::header13& header13;
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

