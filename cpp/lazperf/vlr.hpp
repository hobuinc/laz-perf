/*
===============================================================================

  FILE:  vlr.hpp

  CONTENTS:
    LAZ vlr

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

#include <vector>

#include "lazperf.hpp"

namespace lazperf
{

struct LAZPERF_EXPORT vlr_header
{
    uint16_t reserved;
    std::string user_id; // 16 chars max
    uint16_t record_id;
    uint16_t data_length;
    std::string description; // 32 chars max

    static vlr_header create(std::istream& in);
    void read(std::istream& in);
    void write(std::ostream& out) const;
    static const int Size;
};

struct LAZPERF_EXPORT evlr_header
{
    uint16_t reserved;
    std::string user_id;  // 16 chars max
    uint16_t record_id;
    uint64_t data_length;
    std::string description;  // 32 chars max

    static evlr_header create(std::istream& in);
    void read(std::istream& in);
    void write(std::ostream& out) const;
    static const int Size;
};

struct LAZPERF_EXPORT vlr
{
public:
    virtual ~vlr();
    virtual size_t size() const = 0;
    virtual vlr_header header() const = 0;
};

struct LAZPERF_EXPORT laz_vlr : public vlr
{
public:
    struct LAZPERF_EXPORT laz_item
    {
        uint16_t type;
        uint16_t size;
        uint16_t version;
    };

    uint16_t compressor;
    uint16_t coder;
    uint8_t ver_major;
    uint8_t ver_minor;
    uint16_t revision;
    uint32_t options;
    uint32_t chunk_size;
    uint64_t num_points;
    uint64_t num_bytes;
    std::vector<laz_item> items;

    laz_vlr();
    laz_vlr(int format, int ebCount, uint32_t chunksize);
    virtual ~laz_vlr();

    static laz_vlr create(std::istream& in);
    void read(std::istream& in);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual vlr_header header() const;
};

struct LAZPERF_EXPORT eb_vlr : public vlr
{
public:
    struct LAZPERF_EXPORT ebfield
    {
        uint8_t reserved[2];
        uint8_t data_type;
        uint8_t options;
        std::string name;
        uint8_t unused[4];
        double no_data[3];
        double minval[3];
        double maxval[3];
        double scale[3];
        double offset[3];
        std::string description;

        ebfield();
    };

    std::vector<ebfield> items;

    eb_vlr();
    eb_vlr(int ebCount);
    virtual ~eb_vlr();

    static eb_vlr create(std::istream& in, int byteSize);
    void read(std::istream& in, int byteSize);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual vlr_header header() const;
    void addField();
};

struct LAZPERF_EXPORT wkt_vlr : public vlr
{
public:
    std::string wkt;

    wkt_vlr();
    wkt_vlr(const std::string& s);
    virtual ~wkt_vlr();

    static wkt_vlr create(std::istream& in, int byteSize);
    void read(std::istream& in, int byteSize);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual vlr_header header() const;
};


struct LAZPERF_EXPORT copc_extents_vlr : public lazperf::vlr
{
public:

    struct LAZPERF_EXPORT CopcExtent
    {
        double minimum {0.0};
        double maximum {0.0};

        CopcExtent();
        CopcExtent(double minimum,
                   double maximum);
    };

    std::vector<CopcExtent> items;

    copc_extents_vlr(int itemCount);
    copc_extents_vlr();
    virtual ~copc_extents_vlr();

    static copc_extents_vlr create(std::istream& in, int byteSize);
    void read(std::istream& in, int byteSize);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual lazperf::vlr_header header() const;
};

struct LAZPERF_EXPORT copc_info_vlr : public lazperf::vlr
{
public:
    int64_t span {0};
    uint64_t root_hier_offset {0};
    uint64_t root_hier_size {0};
    uint64_t laz_vlr_offset {0};
    uint64_t laz_vlr_size {0};
    uint64_t wkt_vlr_offset {0};
    uint64_t wkt_vlr_size {0};
    uint64_t eb_vlr_offset {0};
    uint64_t eb_vlr_size {0};
    uint64_t extent_vlr_offset {0};
    uint64_t extent_vlr_size {0};
    double center_x {0.0};
    double center_y {0.0};
    double center_z {0.0};
    double halfsize {0.0};

    uint64_t reserved[5] {0};

    copc_info_vlr();
    virtual ~copc_info_vlr();

    static copc_info_vlr create(std::istream& in);
    void read(std::istream& in);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual lazperf::vlr_header header() const;
};


// This was a COPC draft VLR definition
struct LAZPERF_EXPORT copc_vlr : public vlr
{
public:
    int64_t span {0};
    uint64_t root_hier_offset {0};
    uint64_t root_hier_size {0};
    uint64_t laz_vlr_offset {0};
    uint64_t laz_vlr_size {0};
    uint64_t wkt_vlr_offset {0};
    uint64_t wkt_vlr_size {0};
    uint64_t eb_vlr_offset {0};
    uint64_t eb_vlr_size {0};
    uint64_t reserved[11] {0};

    copc_vlr();
    virtual ~copc_vlr();

    static copc_vlr create(std::istream& in);
    void read(std::istream& in);
    void write(std::ostream& out) const;
    virtual size_t size() const;
    virtual vlr_header header() const;
};

} // namesapce lazperf

