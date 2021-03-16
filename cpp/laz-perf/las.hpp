/*
===============================================================================

  FILE:  las.hpp

  CONTENTS:
    Point formats for LAS

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
#define __las_hpp__

#include <stdint.h>

#include "encoder.hpp"
#include "formats.hpp"
#include "model.hpp"
#include "compressor.hpp"
#include "streams.hpp"
#include "util.hpp"

namespace lazperf
{
namespace las
{

#pragma pack(push, 1)
struct point10
{
    int x;
    int y;
    int z;
    unsigned short intensity;
    unsigned char return_number : 3;
    unsigned char number_of_returns_of_given_pulse : 3;
    unsigned char scan_direction_flag : 1;
    unsigned char edge_of_flight_line : 1;
    unsigned char classification;
    char scan_angle_rank;
    unsigned char user_data;
    unsigned short point_source_ID;

    point10() : x(0), y(0), intensity(0), return_number(0),
        number_of_returns_of_given_pulse(0), scan_direction_flag(0),
        edge_of_flight_line(0), classification(0),
        scan_angle_rank(0), user_data(0), point_source_ID(0)
    {}

    point10(const char *buf)
    {
        unpack(buf);
    }

    void unpack(const char *c)
    {
        x = packers<int>::unpack(c);                     c += sizeof(int);
        y = packers<int>::unpack(c);                     c += sizeof(int);
        z = packers<int>::unpack(c);                     c += sizeof(int);
        intensity = packers<unsigned short>::unpack(c);  c += sizeof(unsigned short);

        to_bitfields(packers<unsigned char>::unpack(c)); c += sizeof(unsigned char);

        classification = packers<unsigned char>::unpack(c);
                                                         c += sizeof(unsigned char);
        scan_angle_rank = packers<char>::unpack(c);      c += sizeof(char);
        user_data = packers<char>::unpack(c);            c += sizeof(char);
        point_source_ID = packers<unsigned short>::unpack(c);
    }

    void pack(char *c)
    {
        packers<int>::pack(x, c);                            c += sizeof(int);
        packers<int>::pack(y, c);                            c += sizeof(int);
        packers<int>::pack(z, c);                            c += sizeof(int);
        packers<unsigned short>::pack(intensity, c);         c += sizeof(unsigned short);
        packers<unsigned char>::pack(from_bitfields(), c);   c += sizeof(unsigned char);
        packers<unsigned char>::pack(classification, c);     c += sizeof(unsigned char); 
        packers<char>::pack(scan_angle_rank, c);             c += sizeof(char);
        packers<char>::pack(user_data, c);                   c += sizeof(char);
        packers<unsigned short>::pack(point_source_ID, c);
    }

    void to_bitfields(unsigned char d)
    {
        return_number = d & 0x7;
        number_of_returns_of_given_pulse = (d >> 3) & 0x7;
        scan_direction_flag = (d >> 6) & 0x1;
        edge_of_flight_line = (d >> 7) & 0x1;
    }

    unsigned char from_bitfields() const
    {
        return ((edge_of_flight_line & 0x1) << 7) |
               ((scan_direction_flag & 0x1) << 6) |
               ((number_of_returns_of_given_pulse & 0x7) << 3) |
               (return_number & 0x7);
    }
};

struct gpstime
{
public:
    gpstime() : value(0)
    {}
    gpstime(int64_t v) : value(v)
    {}
    gpstime(const char *c)
    {
        unpack(c);
    }

    void unpack(const char *in)
    {
        uint64_t lower = packers<unsigned int>::unpack(in);
        uint64_t upper = packers<unsigned int>::unpack(in + 4);

        value = ((upper << 32) | lower);
    }

    void pack(char *buffer)
    {
        packers<unsigned int>::pack(value & 0xFFFFFFFF, buffer);
        packers<unsigned int>::pack(value >> 32, buffer + 4);
    }

    int64_t value;
};

struct rgb
{
public:
    rgb() : r(0), g(0), b(0)
    {}
    rgb(unsigned short r, unsigned short g, unsigned short b) : r(r), g(g), b(b)
    {}
    rgb(const char *buf)
    {
        unpack(buf);
    }

    void unpack(const char *c)
    {
        r = packers<uint16_t>::unpack(c);
        g = packers<uint16_t>::unpack(c + 2);
        b = packers<uint16_t>::unpack(c + 4);
    }

    void pack(char *c)
    {
        packers<uint16_t>::pack(r, c);
        packers<uint16_t>::pack(g, c + 2);
        packers<uint16_t>::pack(b, c + 4);
    }

    uint16_t r;
    uint16_t g;
    uint16_t b;
};

struct rgb14 : public rgb
{
    rgb14()
    {}
    rgb14(const rgb& val) : rgb(val)
    {}
};

            struct nir14
            {
                uint16_t val;

                nir14() : val(0)
                {}

                nir14(uint16_t v) : val(v)
                {}

                nir14(const char *p)
                {
                    unpack(p);
                }

                void pack(char *p)
                {
                    packers<uint16_t>::pack(val, p);
                }

                void unpack(const char *p)
                {
                    val = packers<uint16_t>::unpack(p);
                }
            };

            using byte14 = std::vector<uint8_t>;

            // just the XYZ fields out of the POINT10 struct
            struct xyz {
                int x, y, z;

                xyz() : x(0), y(0), z(0)
                {}
            };

            struct extrabytes : public std::vector<uint8_t>
            {};

            struct point14
            {
                int32_t x_;
                int32_t y_;
                int32_t z_;
                uint16_t intensity_;
                uint8_t returns_;
                uint8_t flags_;
                uint8_t classification_;
                uint8_t user_data_;
                int16_t scan_angle_;
                uint16_t point_source_ID_;
                double gpstime_;

                point14()
                {}

                point14(const char *c)
                {
                    unpack(c);
                }

                int32_t x() const
                { return x_; }
                void setX(int32_t x)
                { x_ = x; }

                int32_t y() const
                { return y_; }
                void setY(int32_t y)
                { y_ = y; }

                int32_t z() const
                { return z_; }
                void setZ(int32_t z)
                { z_ = z; }

                uint16_t intensity() const
                { return intensity_; }
                void setIntensity(uint16_t intensity)
                { intensity_ = intensity; }

                uint8_t returns() const
                { return returns_; }
                void setReturns(uint8_t returns)
                { returns_ = returns; }

                int returnNum() const
                { return returns_ & 0xF; }
                void setReturnNum(int rn)
                { returns_ = rn | (returns_ & 0xF0); }

                uint8_t numReturns() const
                { return returns_ >> 4; }
                void setNumReturns(int nr)
                { returns_ = (nr << 4) | (returns_ & 0xF); }

                uint8_t flags() const
                { return flags_; }
                void setFlags(uint8_t flags)
                { flags_ = flags; }

                int classFlags() const
                { return (flags_ & 0xF); }
                void setClassFlags(int flags)
                { flags_ = flags | (flags_ & 0xF0); }

                int scannerChannel() const
                { return (flags_ >> 4) & 0x03; }
                void setScannerChannel(int c)
                { flags_ = (c << 4) | (flags_ & ~0x30); }

                int scanDirFlag() const
                { return ((flags_ >> 6) & 1); }
                void setScanDirFlag(int flag)
                { flags_ = (flag << 6) | (flags_ & 0xBF); }

                int eofFlag() const
                { return ((flags_ >> 7) & 1); }
                void setEofFlag(int flag)
                { flags_ = (flag << 7) | (flags_ & 0x7F); }

                uint8_t classification() const
                { return classification_; }
                void setClassification(uint8_t classification)
                { classification_ = classification; }

                uint8_t userData() const
                { return user_data_; }
                void setUserData(uint8_t user_data)
                { user_data_ = user_data; }

                int16_t scanAngle() const
                { return scan_angle_; }
                void setScanAngle(int16_t scan_angle)
                { scan_angle_ = scan_angle; }

                uint16_t pointSourceID() const
                { return point_source_ID_; }
                void setPointSourceID(uint16_t point_source_ID)
                { point_source_ID_ = point_source_ID; }

                double gpsTime() const
                { return gpstime_; }
                uint64_t uGpsTime() const
                { return *reinterpret_cast<const uint64_t *>(&gpstime_); }
                int64_t iGpsTime() const
                { return *reinterpret_cast<const int64_t *>(&gpstime_); }
                void setGpsTime(double gpstime)
                { gpstime_ = gpstime; }

                void unpack(const char *in)
                {
                    setX(packers<int32_t>::unpack(in));              in += sizeof(int32_t);
                    setY(packers<int32_t>::unpack(in));              in += sizeof(int32_t);
                    setZ(packers<int32_t>::unpack(in));              in += sizeof(int32_t);
                    setIntensity(packers<uint16_t>::unpack(in));     in += sizeof(uint16_t);
                    setReturns(packers<uint8_t>::unpack(in));        in += sizeof(uint8_t);
                    setFlags(packers<uint8_t>::unpack(in));          in += sizeof(uint8_t);
                    setClassification(packers<uint8_t>::unpack(in)); in += sizeof(uint8_t);
                    setUserData(packers<uint8_t>::unpack(in));       in += sizeof(uint8_t);
                    setScanAngle(packers<int16_t>::unpack(in));      in += sizeof(int16_t);
                    setPointSourceID(packers<uint16_t>::unpack(in)); in += sizeof(uint16_t);
                    setGpsTime(packers<double>::unpack(in));
                }

                void pack(char *c)
                {
                    packers<uint32_t>::pack(x(), c);              c += sizeof(int32_t);
                    packers<uint32_t>::pack(y(), c);              c += sizeof(int32_t);
                    packers<uint32_t>::pack(z(), c);              c += sizeof(int32_t);
                    packers<uint16_t>::pack(intensity(), c);      c += sizeof(uint16_t);
                    packers<uint8_t>::pack(returns(), c);         c += sizeof(uint8_t);
                    packers<uint8_t>::pack(flags(), c);           c += sizeof(uint8_t);
                    packers<uint8_t>::pack(classification(), c);  c += sizeof(uint8_t);
                    packers<uint8_t>::pack(userData(), c);        c += sizeof(uint8_t);
                    packers<int16_t>::pack(scanAngle(), c);       c += sizeof(int16_t);
                    packers<uint16_t>::pack(pointSourceID(), c);  c += sizeof(uint16_t);
                    packers<double>::pack(gpsTime(), c);
                }
            };
#pragma pack(pop)
} // namespace las
} // namespace lazperf

#include "detail/field_byte10.hpp"
#include "detail/field_point10.hpp"
#include "detail/field_point14.hpp"
#include "detail/field_gpstime10.hpp"
#include "detail/field_rgb10.hpp"
#include "detail/field_rgb14.hpp"
#include "detail/field_nir14.hpp"
#include "detail/field_byte14.hpp"

namespace lazperf
{
namespace las
{

// Compressor

struct point_compressor_base_1_2 : public las_compressor
{
    point_compressor_base_1_2(OutputCb cb, int ebCount) :
        stream_(cb), encoder_(stream_), point_(encoder_), extrabytes_(encoder_, ebCount)
    {}

    virtual void done()
    { encoder_.done(); }

    OutCbStream stream_;
    encoders::arithmetic<OutCbStream> encoder_;
    detail::Point10Compressor point_;
    detail::Byte10Compressor extrabytes_;
};

struct point_compressor_0 : public point_compressor_base_1_2
{
    point_compressor_0(OutputCb cb, int ebCount = 0) : point_compressor_base_1_2(cb, ebCount)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compress(in);
        in = this->extrabytes_.compress(in);
        return in;
    }
};

struct point_compressor_1 : public point_compressor_base_1_2
{
    point_compressor_1(OutputCb cb, int ebCount = 0) : point_compressor_base_1_2(cb, ebCount),
        gpstime_(encoder_)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compress(in);
        in = gpstime_.compress(in);
        in = this->extrabytes_.compress(in);
        return in;
    }

private:
    detail::Gpstime10Compressor gpstime_;
};

struct point_compressor_2 : public point_compressor_base_1_2
{
    point_compressor_2(OutputCb cb, int ebCount = 0) : point_compressor_base_1_2(cb, ebCount),
        rgb_(encoder_)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compress(in);
        in = rgb_.compress(in);
        in = this->extrabytes_.compress(in);
        return in;
    }

private:
    detail::Rgb10Compressor rgb_;
};

struct point_compressor_3 : public point_compressor_base_1_2
{
    point_compressor_3(OutputCb cb, int ebCount = 0) : point_compressor_base_1_2(cb, ebCount),
        gpstime_(encoder_), rgb_(encoder_)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compress(in);
        in = gpstime_.compress(in);
        in = rgb_.compress(in);
        in = this->extrabytes_.compress(in);
        return in;
    }

private:
    detail::Gpstime10Compressor gpstime_;
    detail::Rgb10Compressor rgb_;
};

struct point_compressor_base_1_4 : public las_compressor
{
    point_compressor_base_1_4(OutputCb cb, int ebCount) :
        cbStream_(cb), point_(cbStream_), eb_(ebCount, cbStream_), chunk_count_(0)
    {}

    OutCbStream cbStream_;
    lazperf::detail::Point14Compressor point_;
    lazperf::detail::Byte14Compressor eb_;
    uint32_t chunk_count_;
};


struct point_compressor_6 : public point_compressor_base_1_4
{
    point_compressor_6(OutputCb cb, int ebCount = 0) : point_compressor_base_1_4(cb, ebCount)
    {}

    virtual const char *compress(const char *in)
    {
        int channel = 0;
        this->chunk_count_++;
        in = this->point_.compress(in, channel);
        if (this->eb_.count())
            in = this->eb_.compress(in, channel);
        return in;
    }

    virtual void done()
    {
        //ABELL - This probably needs to be byte-ordered.
        //ABELL - There is only one chunk count, even if there are many field<>s in the output.
        cbStream_ << this->chunk_count_;

        this->point_.writeSizes();
        if (this->eb_.count())
            this->eb_.writeSizes();

        this->point_.writeData();
        if (this->eb_.count())
            this->eb_.writeData();
    }
};

struct point_compressor_7 : public point_compressor_base_1_4
{
    lazperf::detail::Rgb14Compressor rgb_;

    point_compressor_7(OutputCb cb, int ebCount = 0) : point_compressor_base_1_4(cb, ebCount),
        rgb_(this->cbStream_)
    {}

    virtual const char *compress(const char *in)
    {
        int channel = 0;
        this->chunk_count_++;
        in = this->point_.compress(in, channel);
        in = rgb_.compress(in, channel);
        if (this->eb_.count())
            in = this->eb_.compress(in, channel);
        return in;
    }

    virtual void done()
    {
        cbStream_ << this->chunk_count_;
        this->point_.writeSizes();
        rgb_.writeSizes();
        if (this->eb_.count())
            this->eb_.writeSizes();

        this->point_.writeData();
        rgb_.writeData();
        if (this->eb_.count())
            this->eb_.writeData();
    }
};

struct point_compressor_8 : public point_compressor_base_1_4
{
    lazperf::detail::Rgb14Compressor rgb_;
    lazperf::detail::Nir14Compressor nir_;

    point_compressor_8(OutputCb cb, int ebCount = 0) : point_compressor_base_1_4(cb, ebCount),
        rgb_(this->cbStream_), nir_(this->cbStream_)
    {}

    virtual const char *compress(const char *in)
    {
        int channel = 0;
        this->chunk_count_++;
        in = this->point_.compress(in, channel);
        in = rgb_.compress(in, channel);
        in = nir_.compress(in, channel);
        if (this->eb_.count())
            in = this->eb_.compress(in, channel);

        return in;
    }

    virtual void done()
    {
        //ABELL - This probably needs to be byte-ordered.
        cbStream_ << this->chunk_count_;
        this->point_.writeSizes();
        rgb_.writeSizes();
        nir_.writeSizes();
        if (this->eb_.count())
            this->eb_.writeSizes();

        this->point_.writeData();
        rgb_.writeData();
        nir_.writeData();
        if (this->eb_.count())
            this->eb_.writeData();
    }
};


// Decompressor

struct point_decompressor_base_1_2 : public las_decompressor
{
    point_decompressor_base_1_2(InputCb cb, int ebCount) :
        stream_(cb), decoder_(stream_), point_(decoder_), extrabytes_(decoder_, ebCount),
        first_(true)
    {}

    void handleFirst()
    {
        if (first_)
        {
            decoder_.readInitBytes();
            first_ = false;
        }
    }

    InCbStream stream_;
    decoders::arithmetic<InCbStream> decoder_;
    detail::Point10Decompressor point_;
    detail::Byte10Decompressor extrabytes_;
    bool first_;
};

struct point_decompressor_0 : public point_decompressor_base_1_2
{
    point_decompressor_0(InputCb cb, int ebCount = 0) : point_decompressor_base_1_2(cb, ebCount)
    {}

    virtual char *decompress(char *in)
    {
        in = this->point_.decompress(in);
        in = this->extrabytes_.decompress(in);
        this->handleFirst();
        return in;
    }
};

struct point_decompressor_1 : public point_decompressor_base_1_2
{
    point_decompressor_1(InputCb cb, int ebCount = 0) : point_decompressor_base_1_2(cb, ebCount),
        gpstime_(decoder_)
    {}

    virtual char *decompress(char *out)
    {
        out = this->point_.decompress(out);
        out = gpstime_.decompress(out);
        out = this->extrabytes_.decompress(out);
        this->handleFirst();
        return out;
    }
private:
    detail::Gpstime10Decompressor gpstime_;
};

struct point_decompressor_2 : public point_decompressor_base_1_2
{
    point_decompressor_2(InputCb cb, int ebCount = 0) : point_decompressor_base_1_2(cb, ebCount),
        rgb_(decoder_)
    {}

    virtual char *decompress(char *out)
    {
        out = this->point_.decompress(out);
        out = rgb_.decompress(out);
        out = this->extrabytes_.decompress(out);
        this->handleFirst();
        return out;
    }

private:
    detail::Rgb10Decompressor rgb_;
};

struct point_decompressor_3 : public point_decompressor_base_1_2
{
    point_decompressor_3(InputCb cb, int ebCount = 0) : point_decompressor_base_1_2(cb, ebCount),
        rgb_(decoder_), gpstime_(decoder_)
    { std::cerr << "Test decompress 3 with ebCount = " << ebCount << "!\n";
    }

    virtual char *decompress(char *out)
    {
        out = this->point_.decompress(out);
        out = gpstime_.decompress(out);
        out = rgb_.decompress(out);
        out = this->extrabytes_.decompress(out);
        this->handleFirst();
        return out;
    }

private:
    detail::Rgb10Decompressor rgb_;
    detail::Gpstime10Decompressor gpstime_;
};

struct point_decompressor_base_1_4 : public las_decompressor
{
    point_decompressor_base_1_4(InputCb cb, int ebCount) : cbStream_(cb), point_(cbStream_),
        eb_(ebCount, cbStream_), first_(true)
    {}

    InCbStream cbStream_;
    lazperf::detail::Point14Decompressor point_;
    lazperf::detail::Byte14Decompressor eb_;
    uint32_t chunk_count_;
    bool first_;
};

struct point_decompressor_6 : public point_decompressor_base_1_4
{
    point_decompressor_6(InputCb cb, int ebCount = 0) : point_decompressor_base_1_4(cb, ebCount)
    {}

    ~point_decompressor_6()
    {
#ifndef NDEBUG
        this->point_.dumpSums();
        if (this->eb_.count())
            this->eb_.dumpSums();
        std::cerr << "\n";
#endif
    }

    virtual char *decompress(char *out)
    {
        int channel = 0;
        out = this->point_.decompress(out, channel);
        if (this->eb_.count())
            out = this->eb_.decompress(out, channel);

        if (this->first_)
        {
            // Read the point count the streams for each data member.
            cbStream_ >> this->chunk_count_;
            this->point_.readSizes();
            if (this->eb_.count())
                this->eb_.readSizes();

            this->point_.readData();
            if (this->eb_.count())
                this->eb_.readData();
            this->first_ = false;
        }

        return out;
    }
};

struct point_decompressor_7 : public point_decompressor_base_1_4
{
    point_decompressor_7(InputCb cb, int ebCount = 0) : point_decompressor_base_1_4(cb, ebCount),
        rgb_(this->cbStream_)
    {}

    ~point_decompressor_7()
    {
#ifndef NDEBUG
        this->point_.dumpSums();
        rgb_.dumpSums();
        if (this->eb_.count())
            this->eb_.dumpSums();
        std::cerr << "\n";
#endif
    }

    virtual char *decompress(char *out)
    {
        int channel = 0;
        out = this->point_.decompress(out, channel);
        out = rgb_.decompress(out, channel);
        if (this->eb_.count())
            out = this->eb_.decompress(out, channel);

        if (this->first_)
        {
            // Read the point count the streams for each data member.
            cbStream_ >> this->chunk_count_;
            this->point_.readSizes();
            rgb_.readSizes();
            if (this->eb_.count())
                this->eb_.readSizes();

            this->point_.readData();
            rgb_.readData();
            if (this->eb_.count())
                this->eb_.readData();
            this->first_ = false;
        }

        return out;
    }

    lazperf::detail::Rgb14Decompressor rgb_;
};

struct point_decompressor_8 : public point_decompressor_base_1_4
{
    point_decompressor_8(InputCb cb, int ebCount = 0) : point_decompressor_base_1_4(cb, ebCount),
        rgb_(this->cbStream_), nir_(this->cbStream_)
    {}

    ~point_decompressor_8()
    {
#ifndef NDEBUG
        this->point_.dumpSums();
        rgb_.dumpSums();
        nir_.dumpSums();
        if (this->eb_.count())
            this->eb_.dumpSums();
        std::cerr << "\n";
#endif
    }

    virtual char *decompress(char *out)
    {
        int channel = 0;
        out = this->point_.decompress(out, channel);
        out = rgb_.decompress(out, channel);
        out = nir_.decompress(out, channel);
        if (this->eb_.count())
            out = this->eb_.decompress(out, channel);

        if (this->first_)
        {
            // Read the point count the streams for each data member.
            cbStream_ >> this->chunk_count_;
            this->point_.readSizes();
            rgb_.readSizes();
            nir_.readSizes();
            if (this->eb_.count())
                this->eb_.readSizes();

            this->point_.readData();
            rgb_.readData();
            nir_.readData();
            if (this->eb_.count())
                this->eb_.readData();
            this->first_ = false;
        }

        return out;
    }

    lazperf::detail::Rgb14Decompressor rgb_;
    lazperf::detail::Nir14Decompressor nir_;
};

} // namespace las
} // namespace lazperf

#endif // __las_hpp__
