/*
===============================================================================

  FILE:  field_byte14.hpp
  
  CONTENTS:
    

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

#ifndef __las_hpp__
#error Cannot directly include this file, this is a part of las.hpp
#endif

namespace laszip {
namespace formats {

template<>
struct field<las::byte14>
{
    typedef las::byte14 type;

  Summer sumByte;

    field(size_t count): count_(count), last_channel_(-1),
        chan_ctxs_{count_, count_, count_, count_}, valid_(count_), byte_cnt_(count_),
        byte_enc_(count_, encoders::arithmetic<MemoryStream>(true)),
        byte_dec_(count_, decoders::arithmetic<MemoryStream>())
    {}

    void dumpSums()
    {
        std::cout << "BYTE     : " << sumByte.value() << "\n";
    }

    template <typename TStream>
    void writeSizes(TStream& stream)
    {
        for (size_t i = 0; i < count_; ++i)
        {
            if (valid_[i])
            {
                byte_enc_[i].done();
                stream << byte_enc_[i].num_encoded();
            }
            else
                stream << (uint32_t)0;
        }
    }


    template <typename TStream>
    void writeData(TStream& stream)
    {

auto sum = [](const uint8_t *buf, uint32_t size)
{
    int32_t sum = 0;
    while (size--)
        sum += *buf++;
    return sum;
};

    int32_t total = 0;
    for (size_t i = 0; i < count_; ++i)
    {
        if (valid_[i])
        {
            stream.putBytes(byte_enc_[i].encoded_bytes(), byte_enc_[i].num_encoded());
            total += sum(byte_enc_[i].encoded_bytes(), byte_enc_[i].num_encoded());
        }
    }
std::cerr << "BYTE      : " << total << "\n";
    }

    template <typename TStream>
    inline const char *compressWith(TStream& stream, const char *buf, int& sc)
    {
        // don't have the first data yet, just push it to our
        // have last stuff and move on
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.putBytes(buf, count_);
            c.last_.assign(buf, buf + count_);
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + count_;
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::byte14 *pLastBytes = &chan_ctxs_[last_channel_].last_;
        if (!c.have_last_)
        {
            c.have_last_ = true;
            pLastBytes = &c.last_;
        }
        // This mess is because of the broken-ness of the handling for last in v3, where
        // 'last_point' only gets updated on the first context switch in the LASzip code.
        las::byte14& lastBytes = *pLastBytes;

        for (size_t i = 0; i < count_; ++i, ++buf)
        {
            int32_t diff = *buf - lastBytes[i];
            byte_enc_[i].encodeSymbol(c.byte_model_[i], diff);
            if (diff)
            {
                valid_[i] = true;
                lastBytes[i] = *buf;
            }
        }

        last_channel_ = sc;
        return buf + count_;
    }

    template <typename TStream>
    void readSizes(TStream& stream)
    {
        for (size_t i = 0; i < count_; ++i)
            stream >> byte_cnt_[i];
    }

    template <typename TStream>
    void readData(TStream& stream)
    {
        for (size_t i = 0; i < count_; ++i)
            byte_dec_[i].initStream(stream, byte_cnt_[i]);
    }

    template<typename TStream>
    inline char *decompressWith(TStream& stream, char *buf, int& sc)
    {
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.getBytes(c.last_.data(), count_);
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + count_;
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::byte14 *pLastByte = &chan_ctxs_[last_channel_].last_;
        if (sc != last_channel_)
        {
            last_channel_ = sc;
            if (!c.have_last_)
            {
                c.have_last_ = true;
                c.last_ = *pLastByte;
                pLastByte = &chan_ctxs_[last_channel_].last_;
            }
        }
        las::byte14& lastByte = *pLastByte;

        for (size_t i = 0; i < count_; ++i, buf++)
        {
            if (byte_cnt_[i])
            {
                *buf = lastByte[i] + byte_dec_[i].decodeSymbol(c.byte_model_[i]);
                lastByte[i] = *buf;
            }
            else
                *buf = lastByte[i];
        }
sumByte.add(lastByte);

        return buf;
    }


    struct ChannelCtx
    {
        int have_last_;
        las::byte14 last_;
        std::vector<models::arithmetic> byte_model_;

        ChannelCtx(size_t count) : have_last_{false}, byte_model_(count, models::arithmetic(256))
        {}
    };

    size_t count_;
    int last_channel_;
    std::array<ChannelCtx, 4> chan_ctxs_;
    std::vector<bool> valid_;
    std::vector<uint32_t> byte_cnt_;
    std::vector<encoders::arithmetic<MemoryStream>> byte_enc_;
    std::vector<decoders::arithmetic<MemoryStream>> byte_dec_;
};

} // namespace formats
} // namespace laszip
