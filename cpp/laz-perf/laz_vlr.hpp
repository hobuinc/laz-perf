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
        std::copy(data, data + sizeof(compressor), (char *)&compressor);
        compressor = le16toh(compressor);
        data += sizeof(compressor);

        std::copy(data, data + sizeof(coder), (char *)&coder);
        coder = le16toh(coder);
        data += sizeof(coder);

        ver_major = *(const unsigned char *)data++;
        ver_minor = *(const unsigned char *)data++;

        std::copy(data, data + sizeof(revision), (char *)&revision);
        revision = le16toh(revision);
        data += sizeof(revision);

        std::copy(data, data + sizeof(options), (char *)&options);
        options = le32toh(options);
        data += sizeof(options);

        std::copy(data, data + sizeof(chunk_size), (char *)&chunk_size);
        chunk_size = le32toh(chunk_size);
        data += sizeof(chunk_size);

        std::copy(data, data + sizeof(num_points), (char *)&num_points);
        num_points = le64toh(num_points);
        data += sizeof(num_points);

        std::copy(data, data + sizeof(num_bytes), (char *)&num_bytes);
        num_bytes = le64toh(num_bytes);
        data += sizeof(num_bytes);

        uint16_t num_items;
        std::copy(data, data + sizeof(num_items), (char *)&num_items);
        num_items = le16toh(num_items);
        data += sizeof(num_items);

        items.clear();
        for (int i = 0 ; i < num_items ; i ++)
        {
            laz_item item;

            std::copy(data, data + sizeof(item.type), (char *)&item.type);
            item.type = le16toh(item.type);
            data += sizeof(item.type);

            std::copy(data, data + sizeof(item.size), (char *)&item.size);
            item.size = le16toh(item.size);
            data += sizeof(item.size);

            std::copy(data, data + sizeof(item.version), (char *)&item.version);
            item.version = le16toh(item.version);
            data += sizeof(item.version);

            items.push_back(item);
        }
    }

    std::vector<uint8_t> data() const
    {
        std::vector<uint8_t> buf(size());

        uint16_t s;
        uint32_t i;
        uint64_t ll;
        char *src;

        char *dst = reinterpret_cast<char *>(buf.data());

        s = htole16(compressor);
        src = (char *)&s;
        std::copy(src, src + sizeof(compressor), dst);
        dst += sizeof(compressor);

        s = htole16(coder);
        src = (char *)&s;
        std::copy(src, src + sizeof(coder), dst);
        dst += sizeof(coder);

        *dst++ = ver_major;
        *dst++ = ver_minor;

        s = htole16(revision);
        src = (char *)&s;
        std::copy(src, src + sizeof(revision), dst);
        dst += sizeof(revision);

        i = htole32(options);
        src = (char *)&i;
        std::copy(src, src + sizeof(options), dst);
        dst += sizeof(options);

        i = htole32(chunk_size);
        src = (char *)&i;
        std::copy(src, src + sizeof(chunk_size), dst);
        dst += sizeof(chunk_size);

        ll = htole64(num_points);
        src = (char *)&ll;
        std::copy(src, src + sizeof(num_points), dst);
        dst += sizeof(num_points);

        ll = htole64(num_bytes);
        src = (char *)&ll;
        std::copy(src, src + sizeof(num_bytes), dst);
        dst += sizeof(num_bytes);

        uint16_t num_items = (uint16_t)items.size();
        s = htole16(num_items);
        src = (char *)&s;
        std::copy(src, src + sizeof(num_items), dst);
        dst += sizeof(num_items);

        for (size_t k = 0 ; k < items.size() ; k++)
        {
            const laz_item& item = items[k];

            s = htole16(item.type);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.type), dst);
            dst += sizeof(item.type);

            s = htole16(item.size);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.size), dst);
            dst += sizeof(item.size);

            s = htole16(item.version);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.version), dst);
            dst += sizeof(item.version);
        }
        return buf;
    }

    /**
    static laz_vlr from_schema(const factory::record_schema& s,
        uint32_t chunksize = DefaultChunkSize)
    {
        laz_vlr r;

        // We only do pointwise chunking.
        r.compressor = (s.format() <= 5) ? 2 : 3;
        r.coder = 0;

        // the version we're compatible with
        r.version.major = 3;
        r.version.minor = 4;
        r.version.revision = 3;

        r.options = 0;
        r.chunk_size = chunksize;

        r.num_points = -1;
        r.num_bytes = -1;

        for (size_t i = 0 ; i < s.records_.size() ; i++)
        {
            auto& rec = s.records_[i];
            laz_item item;

            item.type = static_cast<unsigned short>(rec.type);
            item.size = static_cast<unsigned short>(rec.size);
            item.version = static_cast<unsigned short>(rec.version);
            r.items.push_back(item);
        }
        return r;
    }
    **/

    /**
    // convert the laszip items into record schema to be used by
    // compressor/decompressor
    static factory::record_schema to_schema(const laz_vlr& vlr, int point_len)
    {
        using namespace factory;
        factory::record_schema schema;

        for (size_t i = 0 ; i < vlr.items.size() ; i++)
        {
            const laz_item& item = vlr.items[i];
            schema.push(factory::record_item(item.type, item.size, item.version));
            point_len -= item.size;
        }
        if (point_len < 0)
            schema.invalidate();

        // Add extra bytes information
        if (point_len)
        {
            if (schema.format() <= 5)
                schema.push(factory::record_item(record_item::BYTE, point_len, 2));
            else
                schema.push(factory::record_item(record_item::BYTE14, point_len, 2));
        }
        return schema;
    }
    **/
};
#pragma pack(pop)

} // namesapce lazperf

