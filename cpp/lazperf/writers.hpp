/*
===============================================================================

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

===============================================================================
*/

#pragma once

#include "io.hpp"

namespace lazperf
{
namespace writer
{

class basic_file
{
protected:
    struct Private;

    basic_file();
    virtual ~basic_file();

public:
    LAZPERF_EXPORT void open(std::ostream& out, const io::base_header& h, uint32_t chunk_size);
    LAZPERF_EXPORT void writePoint(const char *p);
    LAZPERF_EXPORT void close();
    LAZPERF_EXPORT uint64_t chunk();
    LAZPERF_EXPORT uint64_t firstChunkOffset() const;
    LAZPERF_EXPORT virtual bool compressed() const;

protected:
    std::unique_ptr<Private> p_; 
};

class named_file : public basic_file
{
    struct Private;

public:
    struct config
    {
    public:
        io::vector3 scale;
        io::vector3 offset;
        unsigned int chunk_size;
        int pdrf;
        int minor_version;
        int extra_bytes;

        explicit config();
        config(const io::vector3& scale, const io::vector3& offset,
            unsigned int chunksize = io::DefaultChunkSize);
        config(const io::base_header& header);

        io::base_header to_header() const;
    };

    LAZPERF_EXPORT named_file(const std::string& filename, const config& c);
    LAZPERF_EXPORT virtual ~named_file();

    LAZPERF_EXPORT void close();

private:
    std::unique_ptr<Private> p_;
};

} // namespace writer
} // namespace lazperf

