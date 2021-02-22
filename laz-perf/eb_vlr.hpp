/*
===============================================================================

  FILE:  eb_vlr.hpp

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

struct eb_vlr : public vlr
{
#pragma pack(push, 1)
    struct eb
    {
        uint8_t reserved[2];
        uint8_t data_type;
        uint8_t options;
        char name[32];
        uint8_t unused[4];
        double no_data[3];
        double minval[3];
        double maxval[3];
        double scale[3];
        double offset[3];
        char description[32];
    };
#pragma pack(pop)

    int cnt;
    std::vector<uint8_t> buf;

    eb_vlr(size_t bytes)
    {
        for (size_t i = 0; i < bytes; ++i)
            addField();
    }

    void addField()
    {
        struct eb field;

        std::string name = "FIELD_" + std::to_string(cnt++);
        strcpy(field.name, name.data());
        field.data_type = htole32(1); // Unsigned char

        uint8_t *pos =  (uint8_t *)&field;
        for (size_t i = 0; i < sizeof(eb); ++i)
            buf.push_back(*pos++);
    }

    size_t size() const
    {
        return buf.size();
    }

    uint8_t *data()
    {
        return buf.data();
    }

    const uint8_t *data() const
    {
        return buf.data();
    }

    virtual vlr::vlr_header header()
    {
        vlr_header h { 0, "LASF_Spec", 4, (uint16_t)size(), ""  };

        return h;
    }
};

} // namesapce io
} // namesapce laszip

