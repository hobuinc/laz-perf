/*
===============================================================================

  FILE:  las.hpp

  CONTENTS:
    Point formats for LAS

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

#include <functional>
#include <memory>
#include <vector>

#define LAZPERF_MAJOR_VERSION 1
#define LAZPERF_MINOR_VERSION 3
#define LAZPERF_REVISION 0
#define LAZPERF_VERSION 1.3.0

namespace lazperf
{

// Called when compressed output is to be written.
using OutputCb = std::function<void(const unsigned char *, size_t)>;

// Called when compressed input is to be read.
using InputCb = std::function<void(unsigned char *, size_t)>;

class las_compressor
{
public:
    typedef std::shared_ptr<las_compressor> ptr;

    virtual const char *compress(const char *in) = 0;
    virtual void done() = 0;
    virtual ~las_compressor();
};

class las_decompressor
{
public:
    typedef std::shared_ptr<las_decompressor> ptr;

    virtual char *decompress(char *in) = 0;
    virtual ~las_decompressor();
};

class point_compressor_base_1_2 : public las_compressor
{
    struct Private;

public:
    void done();

protected:
    point_compressor_base_1_2(OutputCb cb, size_t ebCount);
    virtual ~point_compressor_base_1_2();

    std::unique_ptr<Private> p_;
};

class point_compressor_0 : public point_compressor_base_1_2
{
public:
    point_compressor_0(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
};

class point_compressor_1 : public point_compressor_base_1_2
{
public:
    point_compressor_1(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
};

class point_compressor_2 : public point_compressor_base_1_2
{
public:
    point_compressor_2(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
};

class point_compressor_3 : public point_compressor_base_1_2
{
public:
    point_compressor_3(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
};

class point_compressor_base_1_4 : public las_compressor
{
    struct Private;

public:
    virtual const char *compress(const char *in) = 0;

protected:
    point_compressor_base_1_4(OutputCb cb, size_t ebCount);

    std::unique_ptr<Private> p_;
};


class point_compressor_6 : public point_compressor_base_1_4
{
public:
    point_compressor_6(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
    virtual void done();
};

class point_compressor_7 : public point_compressor_base_1_4
{
public:
    point_compressor_7(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
    virtual void done();
};

class point_compressor_8 : public point_compressor_base_1_4
{
public:
    point_compressor_8(OutputCb cb, size_t ebCount = 0);

    virtual const char *compress(const char *in);
    virtual void done();
};


// Decompressor

class point_decompressor_base_1_2 : public las_decompressor
{
    struct Private;

public:
    virtual char *decompress(char *in) = 0;
    virtual ~point_decompressor_base_1_2();

protected:
    point_decompressor_base_1_2(InputCb cb, size_t ebCount);
    void handleFirst();

    std::unique_ptr<Private> p_;
};

class point_decompressor_0 : public point_decompressor_base_1_2
{
public:
    point_decompressor_0(InputCb cb, size_t ebCount = 0);
    virtual char *decompress(char *in);
};

class point_decompressor_1 : public point_decompressor_base_1_2
{
public:
    point_decompressor_1(InputCb cb, size_t ebCount = 0);
    virtual char *decompress(char *out);
};

class point_decompressor_2 : public point_decompressor_base_1_2
{
public:
    point_decompressor_2(InputCb cb, size_t ebCount = 0);
    virtual char *decompress(char *out);
};

class point_decompressor_3 : public point_decompressor_base_1_2
{
public:
    point_decompressor_3(InputCb cb, size_t ebCount = 0);
    virtual char *decompress(char *out);
};

class point_decompressor_base_1_4 : public las_decompressor
{
    struct Private;

public:
    virtual char *decompress(char *out) = 0;

protected:
    point_decompressor_base_1_4(InputCb cb, size_t ebCount);

    std::unique_ptr<Private> p_;
};

class point_decompressor_6 : public point_decompressor_base_1_4
{
public:
    point_decompressor_6(InputCb cb, size_t ebCount = 0);
    ~point_decompressor_6();

    virtual char *decompress(char *out);
};

class point_decompressor_7 : public point_decompressor_base_1_4
{
public:
    point_decompressor_7(InputCb cb, size_t ebCount = 0);
    ~point_decompressor_7();

    virtual char *decompress(char *out);
};

struct point_decompressor_8 : public point_decompressor_base_1_4
{
public:
    point_decompressor_8(InputCb cb, size_t ebCount = 0);
    ~point_decompressor_8();

    virtual char *decompress(char *out);
};

// FACTORY

las_compressor::ptr build_las_compressor(OutputCb, int format, size_t ebCount = 0);
las_decompressor::ptr build_las_decompressor(InputCb, int format, size_t ebCount = 0);

// CHUNK TABLE

// Note that the chunk values are sizes, rather than offsets.
void compress_chunk_table(OutputCb cb, const std::vector<uint32_t>& chunks);
std::vector<uint32_t> decompress_chunk_table(InputCb cb, size_t numChunks);

} // namespace lazperf

