/*
===============================================================================

  FILE:  vlr.cpp

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

#include <string>

#include "Extractor.hpp"
#include "Inserter.hpp"
#include "charbuf.hpp"
#include "utils.hpp"
#include "vlr.hpp"

namespace lazperf
{

const int vlr_header::Size = 54;

vlr_header vlr_header::create(std::istream& in)
{
    vlr_header h;
    h.read(in);
    return h;
}

void vlr_header::read(std::istream& in)
{
    std::vector<char> buf(Size);
    in.read(buf.data(), buf.size());
    LeExtractor s(buf.data(), buf.size());

    s >> reserved;
    s.get(user_id, 16);
    s >> record_id >> data_length;
    s.get(description, 32);
}

void vlr_header::write(std::ostream& out) const
{
    std::vector<char> buf(Size);
    LeInserter s(buf.data(), buf.size());

    s << reserved;
    s.put(user_id, 16);
    s << record_id << data_length;
    s.put(description, 32);

    out.write(buf.data(), buf.size());
}

///

const int evlr_header::Size = 60;

evlr_header evlr_header::create(std::istream& in)
{
    evlr_header h;
    h.read(in);
    return h;
}

void evlr_header::read(std::istream& in)
{
    std::vector<char> buf(Size);
    in.read(buf.data(), buf.size());
    LeExtractor s(buf.data(), buf.size());

    s >> reserved;
    s.get(user_id, 16);
    s >> record_id >> data_length;
    s.get(description, 32);
}

void evlr_header::write(std::ostream& out) const
{
    std::vector<char> buf(Size);
    LeInserter s(buf.data(), buf.size());

    s << reserved;
    s.put(user_id, 16);
    s << record_id << data_length;
    s.put(description, 32);

    out.write(buf.data(), buf.size());
}


///

vlr::~vlr()
{}


// LAZ VLR

namespace
{
    enum
    {
        BYTE = 0,
        POINT10 = 6,
        GPSTIME = 7,
        RGB12 = 8,
        POINT14 = 10,
        RGB14 = 11,
        RGBNIR14 = 12,
        BYTE14 = 14
    };

    const laz_vlr::laz_item point_item { POINT10, 20, 2 };
    const laz_vlr::laz_item gps_item { GPSTIME, 8, 2 };
    const laz_vlr::laz_item rgb_item { RGB12, 6, 2 };
    const laz_vlr::laz_item byte_item { BYTE, 0, 2 };
    const laz_vlr::laz_item point14_item { POINT14, 30, 3 };
    const laz_vlr::laz_item rgb14_item{ RGB14, 6, 3 };
    const laz_vlr::laz_item rgbnir14_item{ RGBNIR14, 8, 3 };
    const laz_vlr::laz_item byte14_item { BYTE14, 0, 3 };
}

laz_vlr::laz_vlr()
{}

laz_vlr::~laz_vlr()
{}

laz_vlr::laz_vlr(int format, int ebCount, uint32_t chunksize) :
    compressor(format <= 5 ? 2 : 3), coder(0), ver_major(3), ver_minor(4),
    revision(3), options(0), chunk_size(chunksize), num_points(-1),
    num_bytes(-1)
{
    if (format >= 0 && format <= 5)
    {
        items.push_back(point_item);
        if (format == 1 || format == 3)
            items.push_back(gps_item);
        if (format == 2 || format == 3)
            items.push_back(rgb_item);
        if (ebCount)
        {
            laz_vlr::laz_item item(byte_item);
            item.size = ebCount;
            items.push_back(item);
        }
    }
    else if (format >= 6 && format <= 8)
    {
        items.push_back(point14_item);
        if (format == 7)
            items.push_back(rgb14_item);
        if (format == 8)
            items.push_back(rgbnir14_item);
        if (ebCount)
        {
            laz_vlr::laz_item item(byte14_item);
            item.size = ebCount;
            items.push_back(item);
        }
    }
}

size_t laz_vlr::size() const
{
    return 34 + (items.size() * 6);
}

vlr_header laz_vlr::header() const
{
    vlr_header h { 0, "laszip encoded", 22204, (uint16_t)size(), "lazperf variant" };

    return h;
}

laz_vlr laz_vlr::create(std::istream& in)
{
    laz_vlr lazVlr;
    lazVlr.read(in);
    return lazVlr;
}

void laz_vlr::read(std::istream& in)
{
    std::vector<char> buf(34);
    in.read(buf.data(), buf.size());
    LeExtractor s(buf.data(), buf.size());

    uint16_t num_items;

    s >> compressor >> coder >> ver_major >> ver_minor >> revision >> options >>
        chunk_size >> num_points >> num_bytes >> num_items;

    buf.resize(num_items * 6);
    in.read(buf.data(), buf.size());
    LeExtractor s2(buf.data(), buf.size());
    items.clear();
    for (int i = 0; i < num_items; i++)
    {
        laz_item item;

        s2 >> item.type >> item.size >> item.version;
        items.push_back(item);
    }
}

void laz_vlr::write(std::ostream& out) const
{
    std::vector<char> buf(size());
    LeInserter s(buf.data(), buf.size());

    s << compressor << coder << ver_major << ver_minor << revision << options;
    s << chunk_size << num_points << num_bytes << (uint16_t)items.size();
    for (const laz_item& item : items)
        s << item.type << item.size << item.version;
    out.write(buf.data(), buf.size());
}

// Deprecated
std::vector<char> laz_vlr::data() const
{
    std::vector<char> v(size());
    charbuf sbuf(v.data(), v.size());
    std::ostream out(&sbuf);
    write(out);
    return v;
}

// Deprecated
laz_vlr::laz_vlr(const char *d)
{
    uint16_t num_items = le16toh(*reinterpret_cast<const uint16_t *>(d + 32));
    charbuf sbuf(const_cast<char *>(d), 34 + num_items * 6);
    std::istream in(&sbuf);
    read(in);
}

// EB VLR

eb_vlr::ebfield::ebfield() :
    reserved{}, data_type{1}, options{}, unused{},
    no_data{}, minval{}, maxval{}, scale{}, offset{}
{}

eb_vlr::eb_vlr()
{}

eb_vlr::eb_vlr(int ebCount)
{
    while (ebCount--)
        addField();
}

eb_vlr::~eb_vlr()
{}

eb_vlr eb_vlr::create(std::istream& in, int byteSize)
{
    eb_vlr ebVlr;
    ebVlr.read(in, byteSize);
    return ebVlr;
}

void eb_vlr::read(std::istream& in, int byteSize)
{
    std::vector<char> buf(byteSize);
    LeExtractor s(buf.data(), buf.size());
    in.read(buf.data(), buf.size());

    int numItems = byteSize / 192;
    items.clear();
    for (int i = 0; i < numItems; ++i)
    {
        ebfield field;

        s.get(field.reserved, 2);
        s >> field.data_type >> field.options;
        s.get(field.name, 32);
        s.get(field.unused, 4);
        for (int i = 0; i < 3; ++i)
            s >> field.no_data[i];
        for (int i = 0; i < 3; ++i)
            s >> field.minval[i];
        for (int i = 0; i < 3; ++i)
            s >> field.maxval[i];
        for (int i = 0; i < 3; ++i)
            s >> field.scale[i];
        for (int i = 0; i < 3; ++i)
            s >> field.offset[i];
        s.get(field.description, 32);
        items.push_back(field);
    }
}

void eb_vlr::write(std::ostream& out) const
{
    std::vector<char> buf(items.size() * 192);
    LeInserter s(buf.data(), buf.size());

    for (const ebfield& field : items)
    {
        s.put(field.reserved, 2);
        s << field.data_type << field.options;
        s.put(field.name, 32);
        s.put(field.unused, 4);
        for (int i = 0; i < 3; ++i)
            s << field.no_data[i];
        for (int i = 0; i < 3; ++i)
            s << field.minval[i];
        for (int i = 0; i < 3; ++i)
            s << field.maxval[i];
        for (int i = 0; i < 3; ++i)
            s << field.scale[i];
        for (int i = 0; i < 3; ++i)
            s << field.offset[i];
        s.put(field.description, 32);
    }

    out.write(buf.data(), buf.size());
}

void eb_vlr::addField()
{
    ebfield field;

    field.name = "FIELD_" + std::to_string(items.size());
    items.push_back(field);
}

size_t eb_vlr::size() const
{
    return 192 * items.size();
}

vlr_header eb_vlr::header() const
{
    return vlr_header { 0, "LASF_Spec", 4, (uint16_t)size(), ""  };
}

//

wkt_vlr::wkt_vlr()
{}

wkt_vlr::wkt_vlr(const std::string& s) : wkt(s)
{}

wkt_vlr::~wkt_vlr()
{}

wkt_vlr wkt_vlr::create(std::istream& in, int byteSize)
{
    wkt_vlr wktVlr;
    wktVlr.read(in, byteSize);
    return wktVlr;
}

void wkt_vlr::read(std::istream& in, int byteSize)
{
    std::vector<char> buf(byteSize);
    in.read(buf.data(), buf.size());
    wkt.assign(buf.data(), buf.size());
}

void wkt_vlr::write(std::ostream& out) const
{
    out.write(wkt.data(), wkt.size());
}

size_t wkt_vlr::size() const
{
    return wkt.size();
}


vlr_header wkt_vlr::header() const
{
    return vlr_header { 0, "LASF_Projection", 2112, (uint16_t)size(), ""  };
}

//

// Initialized in header.
copc_vlr::copc_vlr()
{}

copc_vlr::~copc_vlr()
{}

copc_vlr copc_vlr::create(std::istream& in)
{
    copc_vlr copcVlr;
    copcVlr.read(in);
    return copcVlr;
}

void copc_vlr::read(std::istream& in)
{
    std::vector<char> buf(size());
    in.read(buf.data(), buf.size());
    LeExtractor s(buf.data(), buf.size());

    s >> span >> root_hier_offset >> root_hier_size;
    s >> laz_vlr_offset >> laz_vlr_size >> wkt_vlr_offset >> wkt_vlr_size;
    s >> eb_vlr_offset >> eb_vlr_size;
    for (int i = 0; i < 11; ++i)
        s >> reserved[i];
}

void copc_vlr::write(std::ostream& out) const
{
    std::vector<char> buf(size());
    LeInserter s(buf.data(), buf.size());

    s << span << root_hier_offset << root_hier_size;
    s << laz_vlr_offset << laz_vlr_size << wkt_vlr_offset << wkt_vlr_size;
    s << eb_vlr_offset << eb_vlr_size;
    for (int i = 0; i < 11; ++i)
        s << reserved[i];
    out.write(buf.data(), buf.size());
}

size_t copc_vlr::size() const
{
    return sizeof(uint64_t) * 20;
}

vlr_header copc_vlr::header() const
{
    return vlr_header { 0, "copc", 1, (uint16_t)size(), "COPC offsets" };
}


copc_extents_vlr::copc_extents_vlr()
{}


copc_extents_vlr::copc_extents_vlr(int itemCount)
{
    items.resize(itemCount);
}


copc_extents_vlr::~copc_extents_vlr()
{}


copc_extents_vlr::CopcExtent::CopcExtent() :
    minimum((std::numeric_limits<double>::max)()),
    maximum((std::numeric_limits<double>::lowest)())
{}


copc_extents_vlr::CopcExtent::CopcExtent(double minimum, double maximum) :
    minimum(minimum),
    maximum(maximum)
{}


copc_extents_vlr copc_extents_vlr::create(std::istream& in, int byteSize)
{
    copc_extents_vlr extentsVlr;
    extentsVlr.read(in, byteSize);
    return extentsVlr;
}


void copc_extents_vlr::read(std::istream& in, int byteSize)
{
    std::vector<char> buf(byteSize);
    LeExtractor s(buf.data(), buf.size());
    in.read(buf.data(), buf.size());

    int numItems = byteSize / (sizeof(double) + sizeof(double));
    items.clear();
    for (int i = 0; i < numItems; ++i)
    {
        CopcExtent field;

        s >> field.minimum >> field.maximum;
        items.push_back(field);
    }
}


void copc_extents_vlr::write(std::ostream& out) const
{
    std::vector<char> buf(size());
    LeInserter s(buf.data(), buf.size());

    for (auto& i: items)
    {
        s << i.minimum << i.maximum;
    }

    out.write(buf.data(), buf.size());
}


size_t copc_extents_vlr::size() const
{
    return items.size() * (sizeof(double) + sizeof(double));
}


lazperf::vlr_header copc_extents_vlr::header() const
{
    return lazperf::vlr_header { 0, "copc", 10000, (uint16_t)size(), "COPC extents" };
}


// Initialized in header.
copc_info_vlr::copc_info_vlr()
{}


copc_info_vlr::~copc_info_vlr()
{}


copc_info_vlr copc_info_vlr::create(std::istream& in)
{
    copc_info_vlr copcVlr;
    copcVlr.read(in);
    return copcVlr;
}


void copc_info_vlr::read(std::istream& in)
{
    std::vector<char> buf(size());
    in.read(buf.data(), buf.size());
    LeExtractor s(buf.data(), buf.size());

    s >> span >> root_hier_offset >> root_hier_size;
    s >> laz_vlr_offset >> laz_vlr_size >> wkt_vlr_offset >> wkt_vlr_size;
    s >> eb_vlr_offset >> eb_vlr_size >> extent_vlr_offset >> extent_vlr_size;
    s >> center_x >> center_y >> center_z >> halfsize;
    for (int i = 0; i < 5; ++i)
        s >> reserved[i];
}


void copc_info_vlr::write(std::ostream& out) const
{
    std::vector<char> buf(size());
    LeInserter s(buf.data(), buf.size());

    s << span << root_hier_offset << root_hier_size;
    s << laz_vlr_offset << laz_vlr_size << wkt_vlr_offset << wkt_vlr_size;
    s << eb_vlr_offset << eb_vlr_size << extent_vlr_offset << extent_vlr_size;
    s << center_x << center_y << center_z << halfsize;
    for (int i = 0; i < 5; ++i)
        s << reserved[i];
    out.write(buf.data(), buf.size());
}


size_t copc_info_vlr::size() const
{
    return sizeof(uint64_t) * 20;
}


lazperf::vlr_header copc_info_vlr::header() const
{
    return lazperf::vlr_header { 0, "copc", 1, (uint16_t)size(), "COPC info" };
}


} // namespace lazperf

