/*
===============================================================================

  FILE:  laz_vlr.hpp

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

#include "vlr.hpp"
#include "utils.hpp"

namespace lazperf
{

#pragma pack(push, 1)

// A Single LAZ Item representation
struct laz_vlr : public vlr
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


    struct laz_item
    {
        uint16_t type;
        uint16_t size;
        uint16_t version;

        static const laz_item point()
        {
            return laz_item{ POINT10, 20, 2 };
        }
        static const laz_item gpstime()
        {
            return laz_item{ GPSTIME, 8, 2 };
        }
        static const laz_item rgb()
        {
            return laz_item{ RGB12, 6, 2 };
        }
        static const laz_item eb(uint16_t count)
        {
            return laz_item{ BYTE, count, 2 };
        }
        static const laz_item point14()
        {
            return laz_item{ POINT14, 30, 3 };
        }
        static const laz_item rgb14()
        {
            return laz_item{ RGB14, 6, 3 };
        }
        static const laz_item rgbnir14()
        {
            return laz_item{ RGBNIR14, 8, 3 };
        }
        static const laz_item eb14(uint16_t count)
        {
            return laz_item{ BYTE14, count, 3 };
        }
    };


    uint16_t compressor;
    uint16_t coder;

    uint8_t ver_major;
    uint8_t ver_minor;
    uint16_t revision;

    uint32_t options;
    uint32_t chunk_size;
    int64_t num_points;
    int64_t num_bytes;

    std::vector<laz_item> items;

    laz_vlr()
    {}

    ~laz_vlr()
    {}

    laz_vlr(int format, int ebCount, uint32_t chunksize) :
        compressor(format <= 5 ? 2 : 3), coder(0), ver_major(3), ver_minor(4),
        revision(3), options(0), chunk_size(chunksize), num_points(-1),
        num_bytes(-1)
    {
        if (format >= 0 && format <= 5)
        {
            items.push_back(laz_item::point());
            if (format == 1 || format == 3)
                items.push_back(laz_item::gpstime());
            if (format == 2 || format == 3)
                items.push_back(laz_item::rgb());
            if (ebCount)
                items.push_back(laz_item::eb(ebCount));
        }
        else if (format >= 6 && format <= 8)
        {
            items.push_back(laz_item::point14());
            if (format == 7)
                items.push_back(laz_item::rgb14());
            if (format == 8)
                items.push_back(laz_item::rgbnir14());
            if (ebCount)
                items.push_back(laz_item::eb14(ebCount));
        }
    }

    laz_vlr(const char *data)
    {
        fill(data);
    }

    virtual size_t size() const
    {
        return 34 + (items.size() * 6);
    }

    virtual vlr::vlr_header header()
    {
        vlr_header h { 0, "laszip encoded", 22204, (uint16_t)size(), "laz-perf variant" };

        return h;
    }

    void fill(const char *data)
    {
        using namespace utils;

        compressor = unpack<uint16_t>(data);          data += sizeof(compressor);
        coder = unpack<uint16_t>(data);               data += sizeof(coder);
        ver_major = *(const unsigned char *)data++;
        ver_minor = *(const unsigned char *)data++;
        revision = unpack<uint16_t>(data);            data += sizeof(revision);
        options = unpack<uint32_t>(data);             data += sizeof(options);
        chunk_size = unpack<uint32_t>(data);          data += sizeof(chunk_size);
        num_points = unpack<int64_t>(data);           data += sizeof(num_points);
        num_bytes = unpack<int64_t>(data);            data += sizeof(num_bytes);

        uint16_t num_items;
        num_items = unpack<uint16_t>(data);           data += sizeof(num_items);
        items.clear();
        for (int i = 0 ; i < num_items; i ++)
        {
            laz_item item;

            item.type = unpack<uint16_t>(data);       data += sizeof(item.type);
            item.size = unpack<uint16_t>(data);       data += sizeof(item.size);
            item.version = unpack<uint16_t>(data);    data += sizeof(item.version);

            items.push_back(item);
        }
    }

    std::vector<uint8_t> data() const
    {
        using namespace utils;

        std::vector<uint8_t> buf(size());
        uint16_t num_items = items.size();

        char *dst = reinterpret_cast<char *>(buf.data());
        pack(compressor, dst);                      dst += sizeof(compressor);
        pack(coder, dst);                           dst += sizeof(coder);
        *dst++ = ver_major;
        *dst++ = ver_minor;
        pack(revision, dst);                        dst += sizeof(revision);
        pack(options, dst);                         dst += sizeof(options);
        pack(chunk_size, dst);                      dst += sizeof(chunk_size);
        pack(num_points, dst);                      dst += sizeof(num_points);
        pack(num_bytes, dst);                       dst += sizeof(num_bytes);
        pack(num_items, dst);                       dst += sizeof(num_items);
        for (size_t k = 0 ; k < items.size() ; k++)
        {
            const laz_item& item = items[k];

            pack(item.type, dst);                   dst += sizeof(item.type);
            pack(item.size, dst);                   dst += sizeof(item.size);
            pack(item.version, dst);                dst += sizeof(item.size);
        }
        return buf;
    }

};
#pragma pack(pop)

} // namesapce lazperf

