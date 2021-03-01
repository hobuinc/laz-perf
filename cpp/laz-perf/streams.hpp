/*
===============================================================================

  FILE:  streams.hpp

  CONTENTS:
    Stream abstractions

  PROGRAMMERS:

    uday.karan@gmail.com - Hobu, Inc.

  COPYRIGHT:

    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  CHANGE HISTORY:

===============================================================================
*/

#ifndef __streams_hpp__
#define __streams_hpp__

#include <algorithm>

#include "portable_endian.hpp"

namespace laszip {

struct MemoryStream
{
    MemoryStream() : buf(), idx(0)
    {}

    void putBytes(const unsigned char* b, size_t len)
    {
        while(len --)
            buf.push_back(*b++);
    }

    void putByte(const unsigned char b)
    {
        buf.push_back(b);
    }

    unsigned char getByte()
    {
        return buf[idx++];
    }

    void getBytes(unsigned char *b, int len)
    {
        for (int i = 0 ; i < len ; i ++)
            b[i] = getByte();
    }

    uint32_t numBytesPut() const
    {
        return buf.size();
    }

    // Copy bytes from the source stream to this stream.
    template <typename TSrc>
    void copy(TSrc& in, size_t bytes)
    {
        buf.resize(bytes);
        in.getBytes(buf.data(), bytes);
    }

    const uint8_t *data() const
    { return buf.data(); }

    std::vector<unsigned char> buf; // cuz I'm ze faste
    size_t idx;
};

template <typename TStream>
TStream& operator << (TStream& stream, uint32_t u)
{
    uint32_t uLe = htole32(u);
    stream.putBytes(reinterpret_cast<const unsigned char *>(&uLe), sizeof(uLe));
    return stream;
}

template <typename TStream>
TStream& operator >> (TStream& stream, uint32_t& u)
{
    uint32_t uLe;
    stream.getBytes(reinterpret_cast<unsigned char *>(&uLe), sizeof(u));
    u = le32toh(uLe);
    return stream;
}

} // namespace laszip

#endif // __streams_hpp__
