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
#include <functional>

#include "excepts.hpp"
#include "portable_endian.hpp"

namespace lazperf
{

using OutputCb = std::function<void(const unsigned char *, size_t)>;
struct OutCbStream
{
    OutCbStream(OutputCb outCb) : outCb_(outCb)
    {}

    void putBytes(const unsigned char *b, size_t len)
    {
        outCb_(b, len);
    }

    void putByte(const unsigned char b)
    {
        outCb_(&b, 1);
    }

    OutputCb outCb_;
};

using InputCb = std::function<void(unsigned char *b, size_t len)>;
struct InCbStream
{
    InCbStream(InputCb inCb) : inCb_(inCb)
    {}

    unsigned char getByte()
    {
        unsigned char c;
        inCb_(&c, 1);
        return c;
    }

    void getBytes(unsigned char *b, size_t len)
    {
        inCb_(b, len);
    }

    InputCb inCb_;
};

// Convenience class

struct OutFileStream
{
public:
    OutFileStream(std::ostream& out) : f_(out)
    {}

    void putBytes(const unsigned char *c, size_t len)
    {
        f_.write(reinterpret_cast<const char *>(c), len);
    }

    OutputCb cb()
    {
        using namespace std::placeholders;

        return std::bind(&OutFileStream::putBytes, this, _1, _2);
    }

private:
    std::ostream& f_;
};

// Convenience class

struct InFileStream
{
    InFileStream(std::istream& in) : f_(in), buf_(1 << 20), offset_(buf_.size())
    {
        std::cerr << "Made in stream with size = " << (1 << 20) << "!\n";
    }

    void getBytes(unsigned char *buf, size_t request)
    {
        // Almost all requests are size 1.
        if (request == 1)
        {
            if (offset_ >= buf_.size())
                fillit();
            *buf = buf_[offset_++];
        }
        else
        {
            // Use what's left in the buffer, if anything.
            size_t fetchable = (std::min)(buf_.size() - offset_, request);
            std::copy(buf_.data() + offset_, buf_.data() + offset_ + fetchable, buf);
            offset_ += fetchable;
            request -= fetchable;
            if (request)
            {
                fillit();
                std::copy(buf_.data() + offset_, buf_.data() + offset_ + request, buf + fetchable);
                offset_ += request;
            }
        }
    }

    // This will force a fill on the next fetch.
    void reset()
    {
        offset_ = buf_.size();
    }

    InputCb cb()
    {
        using namespace std::placeholders;

        return std::bind(&InFileStream::getBytes, this, _1, _2);
    }

private:
    void fillit()
    {
        offset_ = 0;
        f_.read(reinterpret_cast<char *>(buf_.data()), buf_.size());
        buf_.resize(f_.gcount());

        if (buf_.size() == 0)
            throw error("Unexpected end of file.");
    }

    std::istream& f_;
    std::vector<unsigned char> buf_;
    size_t offset_;
};

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

    OutputCb outCb()
    {
        using namespace std::placeholders;

        return std::bind(&MemoryStream::putBytes, this, _1, _2);
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

    InputCb inCb()
    {
        using namespace std::placeholders;

        return std::bind(&MemoryStream::getBytes, this, _1, _2);
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

} // namespace lazperf

#endif // __streams_hpp__
