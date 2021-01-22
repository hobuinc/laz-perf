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

    std::vector<unsigned char> buf; // cuz I'm ze faste
    size_t idx;
};

} // namespace laszip

#endif // __streams_hpp__
