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

namespace laszip
{
namespace io
{

struct vlr
{
#pragma pack(push, 1)
    struct vlr_header
    {
        uint16_t reserved;
        char user_id[16];
        uint16_t record_id;
        uint16_t record_length_after_header;
        char description[32];

        size_t size() const
        { return sizeof(vlr_header); }
    };
#pragma pack(pop)

    virtual size_t size() const = 0;
    virtual uint8_t *data() = 0;
    virtual const uint8_t *data() const = 0;
    virtual vlr_header header() = 0;
};

} // namesapce io
} // namesapce laszip

