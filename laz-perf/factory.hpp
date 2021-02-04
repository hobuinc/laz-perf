/*
===============================================================================

  FILE:  factory.hpp
  
  CONTENTS:
    Factory to create dynamic compressors and decompressors

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

#ifndef __factory_hpp__
#define __factory_hpp__

#include "formats.hpp"
#include "excepts.hpp"
#include "las.hpp"

#include <sstream>

namespace laszip
{
namespace factory
{

struct record_item
{
    enum
    {
        BYTE = 0,
        POINT10 = 6,
        GPSTIME = 7,
        RGB12 = 8,
        POINT14 = 10
    };

    int type;
    int size;
    int version;

    record_item(int t, int s, int v) : type(t), size(s), version(v)
    {}

    bool operator == (const record_item& other) const
    {
        return (type == other.type && version == other.version && size == other.size);
    }

    bool operator != (const record_item& other) const
    {
        return !(*this == other);
    }

    static const record_item& point()
    {
        static record_item item(POINT10, 20, 2);
        return item;
    }

    static const record_item& gpstime()
    {
        static record_item item(GPSTIME, 8, 2);
        return item;
    }

    static const record_item& rgb()
    {
        static record_item item(RGB12, 6, 2);
        return item;
    }

    static const record_item eb(size_t count)
    {
        return record_item(BYTE, count, 2);
    }

    static const record_item point14()
    {
        static record_item item(POINT14, 30, 3);
        return item;
    }
};

struct record_schema
{
    record_schema() : valid_(false)
    {}

    void push(const record_item& item)
    {
        records_.push_back(item);
        valid_ = (format() != -1);
    }

    record_schema& operator () (const record_item& i)
    {
        push(i);
        return *this;
    }

    int size_in_bytes() const
    {
        int sum = 0;
        for (auto i : records_)
            sum += i.size;

        return sum;
    }

    int format() const
    {
        size_t count = records_.size();
        if (count == 0)
            return -1;

        // Ignore extrabytes record that should be at the end.
        if (extrabytes())
            count--;

        if (count == 0)
            return -1;

        if (count == 1)
        {
            if (records_[0] == record_item::point())
                return 0;
            else if (records_[0] == record_item::point14())
                return 6;
            return -1;
        }

        if (count == 2)
        {
            if (records_[1] == record_item::gpstime())
                return 1;
            else if (records_[1] == record_item::rgb())
                return 2;
        }
        if (count == 3 && records_[1] == record_item::gpstime() &&
                records_[2] == record_item::rgb())
            return 3;
        return -1;
    }

    size_t extrabytes() const
    {
        if (records_.size())
        {
            auto ri = records_.rbegin();
            if (ri->type == record_item::BYTE && ri->version == 2)
                return ri->size;
        }
        return 0;
    }

    void invalidate()
    { valid_ = false; }

    bool valid() const
    { return valid_; }

    bool valid_;
    std::vector<record_item> records_;
};


template<typename TStream>
formats::las_compressor::ptr build_las_compressor(TStream& stream, int format, int ebCount = 0)
{
    using namespace formats;

    las_compressor::ptr compressor;

    switch (format)
    {
    case 0:
        compressor.reset(new las::point_compressor_0<TStream>(stream, ebCount));
        break;
    case 1:
        compressor.reset(new las::point_compressor_1<TStream>(stream, ebCount));
        break;
    case 2:
        compressor.reset(new las::point_compressor_2<TStream>(stream, ebCount));
        break;
    case 3:
        compressor.reset(new las::point_compressor_3<TStream>(stream, ebCount));
        break;
    case 6:
        if (ebCount != 0)
            throw error("Can't create point data format 6 compressor with extra bytes.");
        compressor.reset(new las::point_compressor_6<TStream>(stream));
    }
    return compressor;
}

template<typename TStream>
formats::las_compressor::ptr build_las_compressor(TStream& stream, const record_schema& schema)
{
    if (!schema.valid())
        throw error("Can't create compressor. Unsupported/invalid schema.");
    return build_las_compressor(stream, schema.format(), schema.extrabytes());
}

template<typename TStream>
formats::las_decompressor::ptr build_las_decompressor(TStream& stream, int format, int ebCount = 0)
{
    using namespace formats;

    las_decompressor::ptr decompressor;

    switch (format)
    {
    case 0:
        decompressor.reset(new las::point_decompressor_0<TStream>(stream, ebCount));
        break;
    case 1:
        decompressor.reset(new las::point_decompressor_1<TStream>(stream, ebCount));
        break;
    case 2:
        decompressor.reset(new las::point_decompressor_2<TStream>(stream, ebCount));
        break;
    case 3:
        decompressor.reset(new las::point_decompressor_3<TStream>(stream, ebCount));
        break;
    case 6:
        if (ebCount != 0)
            throw error("Can't create point data format 6 compressor with extra bytes.");
        decompressor.reset(new las::point_decompressor_6<TStream>(stream));
    }
    return decompressor;
}

template<typename TStream>
formats::las_decompressor::ptr build_las_decompressor(TStream& stream, const record_schema& schema)
{
    if (!schema.valid())
        throw error("Can't build decompressor. Invalid/unsupported schema.");
    return build_las_decompressor(stream, schema.format(), schema.extrabytes());
}

} // namespace factory
} // namespace laszip

#endif // __factory_hpp__
