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

template<>
struct packers<las::nir14>
{

inline static las::nir14 unpack(const char *in)
{
    return las::nir14(packers<uint16_t>::unpack(in));
}

inline static void pack(const las::nir14& nir, char *p)
{
    packers<uint16_t>::pack(nir.val, p);
}

};

// Figure how to compress and decompress GPS time fields
//
template<>
struct field<las::nir14>
{
    typedef las::nir14 type;

    utils::Summer sumNir;

    field(): last_channel_(-1)
    {}

    void dumpSums()
    {
        std::cout << "NIR      : " << sumNir.value() << "\n";
    }

    template <typename TStream>
    void writeSizes(TStream& stream)
    {
        nir_enc_.done();
        
        stream << nir_enc_.num_encoded();
    }


    template <typename TStream>
    void writeData(TStream& stream)
    {

        LAZDEBUG(std::cerr << "NIR       : " <<
            utils::sum(nir_enc_.encoded_bytes(), nir_enc_.num_encoded()) << "\n");

        if (nir_enc_.num_encoded())
            stream.putBytes(nir_enc_.encoded_bytes(), nir_enc_.num_encoded());
    }

    template <typename TStream>
    inline const char *compressWith(TStream& stream, const char *buf, int& sc)
    {
        const las::nir14 nir = packers<las::nir14>::unpack(buf);

        // don't have the first data yet, just push it to our
        // have last stuff and move on
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.putBytes((const unsigned char*)&nir, sizeof(las::nir14));
            c.last_ = nir;
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + sizeof(las::nir14);
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::nir14 *pLastNir = &chan_ctxs_[last_channel_].last_;
        if (!c.have_last_)
        {
            c.have_last_ = true;
            c.last_ = *pLastNir;
            pLastNir = &c.last_;
        }
        // This mess is because of the broken-ness of the handling for last in v3, where
        // 'last_point' only gets updated on the first context switch in the LASzip code.
        las::nir14& lastNir = *pLastNir;

        bool lowChange = (lastNir.val & 0xFF) != (nir.val & 0xFF);
        bool highChange = (lastNir.val & 0xFF00) != (nir.val & 0xFF00);
        int32_t sym = lowChange | (highChange << 1);
        if (sym)
            nir_enc_.makeValid();
        nir_enc_.encodeSymbol(c.used_model_, sym);

        if (lowChange)
        {
            int32_t diff =  (nir.val & 0xFF) - (lastNir.val & 0xFF);
            nir_enc_.encodeSymbol(c.diff_model_[0], uint8_t(diff));
        }
        if (highChange)
        {
            int32_t diff =  (nir.val >> 8) - (lastNir.val >> 8);
            nir_enc_.encodeSymbol(c.diff_model_[1], uint8_t(diff));
        }

        lastNir = nir;
        last_channel_ = sc;
        return buf + sizeof(las::nir14);
    }

    template <typename TStream>
    void readSizes(TStream& stream)
    {
        stream >> nir_cnt_;
    }

    template <typename TStream>
    void readData(TStream& stream)
    {
        nir_dec_.initStream(stream, nir_cnt_);
    }

    template<typename TStream>
    inline char *decompressWith(TStream& stream, char *buf, int& sc)
    {
        if (last_channel_ == -1)
        {
            ChannelCtx& c = chan_ctxs_[sc];
            stream.getBytes((unsigned char*)buf, sizeof(las::nir14));
            c.last_ = packers<las::nir14>::unpack(buf);
            c.have_last_ = true;
            last_channel_ = sc;
            return buf + sizeof(las::nir14);
        }
        if (nir_cnt_ == 0)
        {
            las::nir14 *nir = reinterpret_cast<las::nir14 *>(buf);
            *nir = chan_ctxs_[last_channel_].last_;
            return buf + sizeof(las::nir14);
        }

        ChannelCtx& c = chan_ctxs_[sc];
        las::nir14 *pLastNir = &chan_ctxs_[last_channel_].last_;
        if (sc != last_channel_)
        {
            last_channel_ = sc;
            if (!c.have_last_)
            {
                c.have_last_ = true;
                c.last_ = *pLastNir;
                pLastNir = &chan_ctxs_[last_channel_].last_;
            }
        }
        las::nir14& lastNir = *pLastNir;

        uint32_t sym = nir_dec_.decodeSymbol(c.used_model_);

        las::nir14 nir;

        if (sym & (1 << 0))
        {
            uint8_t corr = (uint8_t)nir_dec_.decodeSymbol(c.diff_model_[0]);
            nir.val = static_cast<uint16_t>(U8_FOLD(corr + (lastNir.val & 0xFF)));
        }
        else
            nir.val = lastNir.val & 0xFF;

        if (sym & (1 << 1))
        {
            uint8_t corr = (uint8_t)nir_dec_.decodeSymbol(c.diff_model_[1]);
            nir.val |= (static_cast<uint16_t>(U8_FOLD(corr + (lastNir.val >> 8))) << 8);
        }
        else
            nir.val |= lastNir.val & 0xFF00;
        LAZDEBUG(sumNir.add(nir));

        lastNir = nir;
        packers<las::nir14>::pack(nir, buf);
        return buf + sizeof(las::nir14);
    }

    // All the things we need to compress a point, group them into structs
    // so we don't have too many names flying around

    struct ChannelCtx
    {
        int have_last_;
        las::nir14 last_;
        models::arithmetic used_model_;
        std::array<models::arithmetic, 2> diff_model_;

        ChannelCtx() : have_last_{false}, used_model_(4),
            diff_model_{ models::arithmetic(256), models::arithmetic(256) }
        {}
    };

    uint32_t nir_cnt_;
    std::array<ChannelCtx, 4> chan_ctxs_;
    encoders::arithmetic<MemoryStream> nir_enc_ {false};
    decoders::arithmetic<MemoryStream> nir_dec_;
    int last_channel_;
};

} // namespace formats
} // namespace laszip
