/*
===============================================================================

  FILE:  field_rgb.hpp
  
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

namespace detail
{
    static inline unsigned int color_diff_bits_1_4(const las::rgb14& color, const las::rgb14& last)
    {
        const las::rgb14& a = last;
        const las::rgb14& b = color;

        auto flag_diff = [](unsigned short c1, unsigned short c2, int mask) -> bool
        {
            return ((c1 ^ c2) & mask);
        };

        unsigned int r =
            (flag_diff(a.r, b.r, 0x00FF) << 0) |
            (flag_diff(a.r, b.r, 0xFF00) << 1) |
            (flag_diff(a.g, b.g, 0x00FF) << 2) |
            (flag_diff(a.g, b.g, 0xFF00) << 3) |
            (flag_diff(a.b, b.b, 0x00FF) << 4) |
            (flag_diff(a.b, b.b, 0xFF00) << 5) |
            ((flag_diff(b.r, b.g, 0x00FF) |
              flag_diff(b.r, b.b, 0x00FF) |
              flag_diff(b.r, b.g, 0xFF00) |
              flag_diff(b.r, b.b, 0xFF00)) << 6);
        return r;
    }
}

// Figure how to compress and decompress GPS time fields
//
template<>
struct field<las::rgb14>
{
    typedef las::rgb14 type;

  Summer sumRgb;

    field(): last_channel_(-1)
    {}

    void dumpSums()
    {
        std::cout << "RGB      : " << sumRgb.value() << "\n";
    }

    template <typename TStream>
    void writeSizes(TStream& stream)
    {
        rgb_enc_.done();
        
        stream << rgb_enc_.num_encoded();
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

std::cerr << "RGB       : " << sum(rgb_enc_.encoded_bytes(), rgb_enc_.num_encoded()) << "\n";

        if (rgb_enc_.num_encoded())
            stream.putBytes(rgb_enc_.encoded_bytes(), rgb_enc_.num_encoded());
    }

    template <typename TStream>
    inline const char *compressWith(TStream& stream, const char *buf, int& sc)
    {
        const las::rgb14 color = packers<las::rgb>::unpack(buf);

        // don't have the first data yet, just push it to our
        // have last stuff and move on
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.putBytes((const unsigned char*)&color, sizeof(las::rgb));
            c.last_ = color;
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + sizeof(las::rgb);
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::rgb14 *pLastColor = &chan_ctxs_[last_channel_].last_;
        if (!c.have_last_)
        {
            c.have_last_ = true;
            c.last_ = *pLastColor;
            pLastColor = &c.last_;
        }
        // This mess is because of the broken-ness of the handling for last in v3, where
        // 'last_point' only gets updated on the first context switch in the LASzip code.
        las::rgb& lastColor = *pLastColor;

        // compress color
        int diff_l = 0;
        int diff_h = 0;

        unsigned int sym = detail::color_diff_bits_1_4(color, lastColor);
        if (sym)
            rgb_enc_.makeValid();
        rgb_enc_.encodeSymbol(c.used_model_, sym);

        // high and low R
        if (sym & (1 << 0))
        {
            diff_l = (color.r & 0xFF) - (lastColor.r & 0xFF);
            rgb_enc_.encodeSymbol(c.diff_model_[0], U8_FOLD(diff_l));
        }
        if (sym & (1 << 1))
        {
            diff_h = static_cast<int>(color.r >> 8) - (lastColor.r >> 8);
            rgb_enc_.encodeSymbol(c.diff_model_[1], U8_FOLD(diff_h));
        }

        // Only encode green and blue if they are different from red.
        if (sym & (1 << 6))
        {
            if (sym & (1 << 2))
            {
                int corr = static_cast<int>(color.g & 0xFF) -
                    U8_CLAMP(diff_l + (lastColor.g & 0xFF));
                rgb_enc_.encodeSymbol(c.diff_model_[2], U8_FOLD(corr));
            }

            if (sym & (1 << 4))
            {
                diff_l = (diff_l + (color.g & 0xFF) - (lastColor.g & 0xFF)) / 2;
                int corr = static_cast<int>(color.b & 0xFF) -
                    U8_CLAMP(diff_l + (lastColor.b & 0xFF));
                rgb_enc_.encodeSymbol(c.diff_model_[4], U8_FOLD(corr));
            }

            if (sym & (1 << 3))
            {
                int corr = static_cast<int>(color.g >> 8) - U8_CLAMP(diff_h + (lastColor.g >> 8));
                rgb_enc_.encodeSymbol(c.diff_model_[3], U8_FOLD(corr));
            }

            if (sym & (1 << 5))
            {
                diff_h = (diff_h + ((color.g >> 8)) - (lastColor.g >> 8)) / 2;
                int corr = static_cast<int>(color.b >> 8) - U8_CLAMP(diff_h + (lastColor.b >> 8));
                rgb_enc_.encodeSymbol(c.diff_model_[5], U8_FOLD(corr));
            }
        }

        lastColor = color;
        last_channel_ = sc;
        return buf + sizeof(las::rgb14);
    }

    template <typename TStream>
    void readSizes(TStream& stream)
    {
        stream >> rgb_cnt_;
    }

    template <typename TStream>
    void readData(TStream& stream)
    {
        rgb_dec_.initStream(stream, rgb_cnt_);
    }

    template<typename TStream>
    inline char *decompressWith(TStream& stream, char *buf, int& sc)
    {
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.getBytes((unsigned char*)buf, sizeof(las::rgb));
            c.last_ = packers<las::rgb>::unpack(buf);
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + sizeof(las::rgb14);
        }
        if (rgb_cnt_ == 0)
        {
            las::rgb14 *color = reinterpret_cast<las::rgb14 *>(buf);
            *color = chan_ctxs_[last_channel_].last_;
            return buf + sizeof(las::rgb14);
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::rgb14 *pLastColor = &chan_ctxs_[last_channel_].last_;
        if (sc != last_channel_)
        {
            last_channel_ = sc;
            if (!c.have_last_)
            {
                c.have_last_ = true;
                c.last_ = *pLastColor;
                pLastColor = &chan_ctxs_[last_channel_].last_;
            }
        }
        las::rgb14& lastColor = *pLastColor;

        uint32_t sym = rgb_dec_.decodeSymbol(c.used_model_);

        las::rgb14 color;

        if (sym & (1 << 0))
        {
            uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[0]);
            color.r = static_cast<unsigned short>(U8_FOLD(corr + (lastColor.r & 0xFF)));
        }
        else
            color.r = lastColor.r & 0xFF;

        if (sym & (1 << 1))
        {
            uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[1]);
            color.r |= (static_cast<unsigned short>(U8_FOLD(corr + (lastColor.r >> 8))) << 8);
        }
        else
            color.r |= lastColor.r & 0xFF00;

        if (sym & (1 << 6))
        {
            int diff = (color.r & 0xFF) - (lastColor.r & 0xFF);

            if (sym & (1 << 2))
            {
                uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[2]);
                color.g = static_cast<unsigned short>(U8_FOLD(corr +
                    U8_CLAMP(diff + (lastColor.g & 0xFF))));
            }
            else
                color.g = lastColor.g & 0xFF;

            if (sym & (1 << 4))
            {
                uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[4]);
                diff = (diff + ((color.g & 0xFF) - (lastColor.g & 0xFF))) / 2;
                color.b = static_cast<unsigned short>(U8_FOLD(corr +
                    U8_CLAMP(diff + (lastColor.b & 0xFF))));
            }
            else
                color.b = lastColor.b & 0xFF;

            diff = (color.r >> 8) - (lastColor.r >> 8);
            if (sym & (1 << 3))
            {
                uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[3]);
                color.g |= (static_cast<unsigned short>(U8_FOLD(corr +
                    U8_CLAMP(diff + (lastColor.g >> 8))))) << 8;
            }
            else
                color.g |= lastColor.g & 0xFF00;

            if (sym & (1 << 5))
            {
                uint8_t corr = (uint8_t)rgb_dec_.decodeSymbol(c.diff_model_[5]);
                diff = (diff + (color.g >> 8) - (lastColor.g >> 8)) / 2;
                color.b |= (static_cast<unsigned short>(U8_FOLD(corr +
                    U8_CLAMP(diff + (lastColor.b >> 8))))) << 8;
            }
            else
                color.b |= (lastColor.b & 0xFF00);
        }
        else
        {
            color.g = color.r;
            color.b = color.r;
        }

sumRgb.add(color);
        lastColor = color;
        packers<las::rgb>::pack(color, buf);
        return buf + sizeof(las::rgb14);
    }

    // All the things we need to compress a point, group them into structs
    // so we don't have too many names flying around

    struct ChannelCtx
    {
        int have_last_;
        las::rgb14 last_;
        models::arithmetic used_model_;
        std::array<models::arithmetic, 6> diff_model_;

        ChannelCtx() : have_last_{false}, used_model_(128),
            diff_model_{ models::arithmetic(256), models::arithmetic(256), models::arithmetic(256),
                    models::arithmetic(256), models::arithmetic(256), models::arithmetic(256) }
        {}
    };

    uint32_t rgb_cnt_;
    std::array<ChannelCtx, 4> chan_ctxs_;
    encoders::arithmetic<MemoryStream> rgb_enc_ {false};
    decoders::arithmetic<MemoryStream> rgb_dec_;
    int last_channel_;
};

} // namespace formats
} // namespace laszip
