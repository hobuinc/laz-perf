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

namespace laszip {
	namespace formats {
		namespace las {
#pragma pack(push, 1)
			struct point10 {
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
			};

			struct gpstime {
				int64_t value;

				gpstime() : value(0) {}
				gpstime(int64_t v) : value(v) {}
			};

			struct rgb {
				unsigned short r, g, b;

				rgb(): r(0), g(0), b(0) {}
				rgb(unsigned short _r, unsigned short _g, unsigned short _b) :
					r(_r), g(_g), b(_b) {}
			};

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
            private:
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

            public:
                point14()
                {}

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

                uint8_t numReturns() const
                { return returns_ >> 4; }

                uint8_t flags() const
                { return flags_; }
                void setFlags(uint8_t flags)
                { flags_ = flags; }

                int classFlags() const
                { return (flags_ & 0xF); }

                int scannerChannel() const
                { return (flags_ >> 4) & 0x03; }

                int scanDirFlag() const
                { return ((flags_ >> 6) & 1); }

                int eofFlag() const
                { return ((flags_ >> 7) & 1); }

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
			};
#pragma pack(pop)
		}
	}
}

#include "detail/field_extrabytes.hpp"
#include "detail/field_point10.hpp"
#include "detail/field_point14.hpp"
#include "detail/field_gpstime.hpp"
#include "detail/field_rgb.hpp"
#include "detail/field_xyz.hpp"

namespace laszip
{
namespace formats
{
namespace las
{

// Compressor

template<typename TStream>
struct point_compressor_base_1_2 : public formats::las_compressor
{
    point_compressor_base_1_2(TStream& stream, int ebCount) :
        encoder_(stream), extrabytes_(ebCount)
    {}

    virtual void done()
    { encoder_.done(); }

    encoders::arithmetic<TStream> encoder_;
    field<point10> point_;
    field<extrabytes> extrabytes_;
};

template<typename TStream>
struct point_compressor_base_1_4 : public formats::las_compressor
{
    point_compressor_base_1_4(TStream& stream) : stream_(stream)
    {}

    virtual void done()
    {
        stream_ << chunk_count_;
        point_.done(stream_);
    }

    TStream& stream_;
    field<point14> point_;
    uint32_t chunk_count_;
};

//ABELL - Move this
template<typename TStream>
struct point_compressor_6 : public point_compressor_base_1_4<TStream>
{
    point_compressor_6(TStream& stream) : point_compressor_base_1_4<TStream>(stream)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compressWith(this->stream_, in);
        return in;
    }
};

template<typename TStream>
struct point_compressor_0 : public point_compressor_base_1_2<TStream>
{
    point_compressor_0(TStream& stream, int ebCount) :
        point_compressor_base_1_2<TStream>(stream, ebCount)
    {}

    virtual const char *compress(const char *in)
    {
        in = this->point_.compressWith(this->encoder_, in);
        in = this->extrabytes_.compressWith(this->encoder_, in);
        return in;
    }
};

template<typename TStream>
struct point_compressor_1 : public point_compressor_base_1_2<TStream>
{
    point_compressor_1(TStream& stream, int ebCount) :
        point_compressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<gpstime> gpstime_;

    virtual const char *compress(const char *in)
    {
        in = this->point_.compressWith(this->encoder_, in);
        in = gpstime_.compressWith(this->encoder_, in);
        in = this->extrabytes_.compressWith(this->encoder_, in);
        return in;
    }
};

template<typename TStream>
struct point_compressor_2 : public point_compressor_base_1_2<TStream>
{
    point_compressor_2(TStream& stream, int ebCount) :
        point_compressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<rgb> rgb_;

    virtual const char *compress(const char *in)
    {
        in = this->point_.compressWith(this->encoder_, in);
        in = rgb_.compressWith(this->encoder_, in);
        in = this->extrabytes_.compressWith(this->encoder_, in);
        return in;
    }
};

template<typename TStream>
struct point_compressor_3 : public point_compressor_base_1_2<TStream>
{
    point_compressor_3(TStream& stream, int ebCount) :
        point_compressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<gpstime> gpstime_;
    field<rgb> rgb_;

    virtual const char *compress(const char *in)
    {
        in = this->point_.compressWith(this->encoder_, in);
        in = gpstime_.compressWith(this->encoder_, in);
        in = rgb_.compressWith(this->encoder_, in);
        in = this->extrabytes_.compressWith(this->encoder_, in);
        return in;
    }
};


// Decompressor

template<typename TStream>
struct point_decompressor_base_1_2 : public formats::las_decompressor
{
    point_decompressor_base_1_2(TStream& stream, int ebCount) :
        decoder_(stream), extrabytes_(ebCount), first_(true)
    {}

    //ABELL - This is a bad hack that should by changing the call site.
    void handleFirst()
    {
        if (first_)
        {
            decoder_.readInitBytes();
            first_ = false;
        }
    }

    decoders::arithmetic<TStream> decoder_;
    field<point10> point_;
    field<extrabytes> extrabytes_;
    bool first_;
};

template<typename TStream>
struct point_decompressor_0 : public point_decompressor_base_1_2<TStream>
{
    point_decompressor_0(TStream& stream, int ebCount) :
        point_decompressor_base_1_2<TStream>(stream, ebCount)
    {}

    virtual char *decompress(char *in)
    {
        in = this->point_.decompressWith(this->decoder_, in);
        in = this->extrabytes_.decompressWith(this->decoder_, in);
        this->handleFirst();
        return in;
    }
};

template<typename TStream>
struct point_decompressor_1 : public point_decompressor_base_1_2<TStream>
{
    point_decompressor_1(TStream& stream, int ebCount) :
        point_decompressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<gpstime> gpstime_;

    virtual char *decompress(char *in)
    {
        in = this->point_.decompressWith(this->decoder_, in);
        in = gpstime_.decompressWith(this->decoder_, in);
        in = this->extrabytes_.decompressWith(this->decoder_, in);
        this->handleFirst();
        return in;
    }
};

template<typename TStream>
struct point_decompressor_2 : public point_decompressor_base_1_2<TStream>
{
    point_decompressor_2(TStream& stream, int ebCount) :
        point_decompressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<rgb> rgb_;

    virtual char *decompress(char *in)
    {
        in = this->point_.decompressWith(this->decoder_, in);
        in = rgb_.decompressWith(this->decoder_, in);
        in = this->extrabytes_.decompressWith(this->decoder_, in);
        this->handleFirst();
        return in;
    }
};

template<typename TStream>
struct point_decompressor_3 : public point_decompressor_base_1_2<TStream>
{
    point_decompressor_3(TStream& stream, int ebCount) :
        point_decompressor_base_1_2<TStream>(stream, ebCount)
    {}

    field<gpstime> gpstime_;
    field<rgb> rgb_;

    virtual char *decompress(char *in)
    {
        in = this->point_.decompressWith(this->decoder_, in);
        in = gpstime_.decompressWith(this->decoder_, in);
        in = rgb_.decompressWith(this->decoder_, in);
        in = this->extrabytes_.decompressWith(this->decoder_, in);
        this->handleFirst();
        return in;
    }
};

} // namespace las
} // namespace formats
} // namespace laszip

#endif // __las_hpp__
