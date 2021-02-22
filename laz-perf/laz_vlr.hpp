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

namespace laszip
{
namespace io
{

#pragma pack(push, 1)

// A Single LAZ Item representation
struct laz_item
{
    uint16_t type;
    uint16_t size;
    uint16_t version;
};

struct laz_vlr : public vlr
{
    uint16_t compressor;
    uint16_t coder;

    struct {
        uint8_t major;
        uint8_t minor;
        uint16_t revision;
    } version;

    uint32_t options;
    uint32_t chunk_size;

    int64_t num_points;
    int64_t num_bytes;

    uint16_t num_items;
    laz_item *items;

    laz_vlr() : num_items(0), items(NULL)
    {}

    ~laz_vlr()
    {
        delete [] items;
    }

    laz_vlr(const char *data)
    {
        items = NULL;
        fill(data);
    }

    virtual size_t size() const
    {
        return sizeof(laz_vlr) - sizeof(vlr) - sizeof(laz_item *) +
            (num_items * sizeof(laz_item));
    }

    virtual uint8_t *data()
    {
        return (uint8_t *)this;
    }

    virtual const uint8_t *data() const
    {
        return (const uint8_t *)this;
    }

    virtual vlr::vlr_header header()
    {
        vlr_header h { 0, "laszip encoded", 22204, (uint16_t)size(), "laz-perf variant" };

        return h;
    }

    laz_vlr(const laz_vlr& rhs)
    {
        compressor = rhs.compressor;
        coder = rhs.coder;

        // the version we're compatible with
        version.major = rhs.version.major;
        version.minor = rhs.version.minor;
        version.revision = rhs.version.revision;

        options = rhs.options;
        chunk_size = rhs.chunk_size;

        num_points = rhs.num_points;
        num_bytes = rhs.num_bytes;

        num_items = rhs.num_items;
        if (rhs.items)
        {
            items = new laz_item[num_items];
            for (int i = 0 ; i < num_items ; i ++)
                items[i] = rhs.items[i];
        }
    }

    laz_vlr& operator = (const laz_vlr& rhs)
    {
        if (this == &rhs)
            return *this;

        compressor = rhs.compressor;
        coder = rhs.coder;

        // the version we're compatible with
        version.major = rhs.version.major;
        version.minor = rhs.version.minor;
        version.revision = rhs.version.revision;

        options = rhs.options;
        chunk_size = rhs.chunk_size;

        num_points = rhs.num_points;
        num_bytes = rhs.num_bytes;

        num_items = rhs.num_items;
        if (rhs.items) {
            items = new laz_item[num_items];
            for (int i = 0 ; i < num_items ; i ++)
                items[i] = rhs.items[i];
        }
        return *this;
    }

    void fill(const char *data)
    {
        std::copy(data, data + sizeof(compressor), (char *)&compressor);
        compressor = le16toh(compressor);
        data += sizeof(compressor);

        std::copy(data, data + sizeof(coder), (char *)&coder);
        coder = le16toh(coder);
        data += sizeof(coder);

        version.major = *(const unsigned char *)data++;
        version.minor = *(const unsigned char *)data++;

        std::copy(data, data + sizeof(version.revision), (char *)&version.revision);
        version.revision = le16toh(version.revision);
        data += sizeof(version.revision);

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

        std::copy(data, data + sizeof(num_items), (char *)&num_items);
        num_items = le16toh(num_items);
        data += sizeof(num_items);

        delete [] items;
        items = new laz_item[num_items];
        for (int i = 0 ; i < num_items ; i ++)
        {
            laz_item& item = items[i];

            std::copy(data, data + sizeof(item.type), (char *)&item.type);
            item.type = le16toh(item.type);
            data += sizeof(item.type);

            std::copy(data, data + sizeof(item.size), (char *)&item.size);
            item.size = le16toh(item.size);
            data += sizeof(item.size);

            std::copy(data, data + sizeof(item.version), (char *)&item.version);
            item.version = le16toh(item.version);
            data += sizeof(item.version);
        }
    }

    void extract(char *data)
    {
        uint16_t s;
        uint32_t i;
        uint64_t ll;
        char *src;

        s = htole16(compressor);
        src = (char *)&s;
        std::copy(src, src + sizeof(compressor), data);
        data += sizeof(compressor);

        s = htole16(coder);
        src = (char *)&s;
        std::copy(src, src + sizeof(coder), data);
        data += sizeof(coder);

        *data++ = version.major;
        *data++ = version.minor;

        s = htole16(version.revision);
        src = (char *)&s;
        std::copy(src, src + sizeof(version.revision), data);
        data += sizeof(version.revision);

        i = htole32(options);
        src = (char *)&i;
        std::copy(src, src + sizeof(options), data);
        data += sizeof(options);

        i = htole32(chunk_size);
        src = (char *)&i;
        std::copy(src, src + sizeof(chunk_size), data);
        data += sizeof(chunk_size);

        ll = htole64(num_points);
        src = (char *)&ll;
        std::copy(src, src + sizeof(num_points), data);
        data += sizeof(num_points);

        ll = htole64(num_bytes);
        src = (char *)&ll;
        std::copy(src, src + sizeof(num_bytes), data);
        data += sizeof(num_bytes);

        s = htole16(num_items);
        src = (char *)&s;
        std::copy(src, src + sizeof(num_items), data);
        data += sizeof(num_items);

        for (int k = 0 ; k < num_items ; k ++) {
            laz_item& item = items[k];

            s = htole16(item.type);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.type), data);
            data += sizeof(item.type);

            s = htole16(item.size);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.size), data);
            data += sizeof(item.size);

            s = htole16(item.version);
            src = (char *)&s;
            std::copy(src, src + sizeof(item.version), data);
            data += sizeof(item.version);
        }
    }

    static laz_vlr from_schema(const factory::record_schema& s,
        uint32_t chunksize)
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

        r.num_items = static_cast<unsigned short>(s.records_.size());
        r.items = new laz_item[s.records_.size()];
        for (size_t i = 0 ; i < s.records_.size() ; i ++)
        {
            laz_item& item = r.items[i];
            const factory::record_item& rec = s.records_.at(i);

            item.type = static_cast<unsigned short>(rec.type);
            item.size = static_cast<unsigned short>(rec.size);
            item.version = static_cast<unsigned short>(rec.version);
        }
        return r;
    }

    // convert the laszip items into record schema to be used by
    // compressor/decompressor
    static factory::record_schema to_schema(const laz_vlr& vlr, int point_len)
    {
        using namespace factory;
        factory::record_schema schema;

        for (auto i = 0 ; i < vlr.num_items ; i++)
        {
            laz_item& item = vlr.items[i];
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
};
#pragma pack(pop)

} // namesapce io
} // namesapce laszip

