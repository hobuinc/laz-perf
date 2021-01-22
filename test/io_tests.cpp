/*
===============================================================================

  FILE:  io_tests.cpp

  CONTENTS:
    Factory to create dynamic compressors and decompressors

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

#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <memory>

#include "test_main.hpp"

#include <laz-perf/io.hpp>

#include <cstdio>
#include "reader.hpp"
#include <stdio.h>

struct memory_stream
{

    memory_stream(const char *buf, std::streamsize len) :
        buf_(buf), len_(len), offset_(0),
        is_bad_(false), is_eof_(false), last_read_count_(0)
    {}

    void read(char *into, std::streamsize size)
    {
        if (is_eof_) {
            is_bad_ = true;
            return;
        }

        std::streamsize to_read = (std::min)(size, len_ - offset_);
        std::copy(buf_ + offset_, buf_ + offset_ + to_read, into);
        offset_ += to_read;
        last_read_count_ = to_read;

        if (offset_ >= len_)
            is_eof_ = true;
    }

    bool eof()
    {
        return is_eof_;
    }

    std::streamsize gcount()
    {
        return last_read_count_;
    }

    bool good()
    {
        bool b = is_bad_;
        is_bad_ = false;
        return !b;
    }

    void clear()
    {
        is_bad_ = false;
        is_eof_ = false;
    }

    std::streamsize tellg()
    {
        return offset_;
    }

    void seekg(std::ios::pos_type p)
    {
        if (p >= len_)
            is_bad_ = true;
        else
            offset_ = p;
    }

    void seekg(std::ios::off_type p, std::ios_base::seekdir dir)
    {
        std::streamoff new_offset_ = 0;

        switch(dir) {
            case std::ios::beg:
                new_offset_ = p;
                break;
            case std::ios::end:
                new_offset_ = len_ + p - 1;
                break;
            case std::ios::cur:
                new_offset_ = offset_ + p;
                break;
            default:
                break;
        }

        if (new_offset_ >= len_ || new_offset_ < 0)
            is_bad_ = true;
        else
        {
            is_bad_ = false;
            offset_ = new_offset_;
        }
    }

    const char *buf_;
    std::streamsize len_;
    std::streamsize offset_;
    bool is_bad_;
    bool is_eof_;
    std::streamsize last_read_count_;
};

std::string makeTempFileName()
{
#ifdef _WIN32
    char *fnTemplate = "fnXXXXXX";
    char name[9];
    strcpy_s(name, sizeof(name), fnTemplate);
    size_t size = strnlen(name, 9) + 1;
    _mktemp_s(name, size);
    char path[MAX_PATH];
    GetTempPath(MAX_PATH, path);

    return std::string(path) + std::string(name, 8);
#else
    char name[L_tmpnam];
    std::tmpnam(name);

    return std::string(name);
#endif
}

TEST(io_tests, io_structs_are_of_correct_size) {
    using namespace laszip::io;

    EXPECT_EQ(sizeof(header), 227u);
}

TEST(io_tests, can_report_invalid_magic) {
    using namespace laszip;

    {
        std::ifstream file(testFile("point10-1.las.raw"), std::ios::binary);
        auto func = [&file]() {
            io::reader::file f(file);
        };

        EXPECT_THROW(func(), error);

        file.close();
    }
}

TEST(io_tests, can_check_for_no_compression) {
    using namespace laszip;

    {
        std::ifstream file(testFile("point10.las"), std::ios::binary);
        auto func = [&file]() {
            io::reader::file f(file);
        };
        EXPECT_THROW(func(), error);
        file.close();
    }
}

TEST(io_tests, doesnt_throw_any_errors_for_valid_laz) {
    using namespace laszip;
    {
        std::ifstream file(testFile("point10.las.laz"), std::ios::binary);
        auto func = [&file]() {
            io::reader::file f(file);
        };

        EXPECT_NO_THROW(func());
    }
}

void dumpBytes(const char* b, size_t len) {
    for (size_t i = 0 ; i < len ; i ++) {
        printf("%02X ", (unsigned char)b[i]);

        if ((i+1) % 16 == 0)
            printf("\n");
    }
}

TEST(io_tests, parses_header_correctly) {
    using namespace laszip;

    {
        std::ifstream file(testFile("point10.las.laz"), std::ios::binary);
        io::reader::file f(file);
        auto header = f.get_header();

        EXPECT_EQ(header.version.major, 1);
        EXPECT_EQ(header.version.minor, 2);

        EXPECT_EQ(header.creation.day, 113);
        EXPECT_EQ(header.creation.year, 2014);

        EXPECT_EQ(header.header_size, 227);
        EXPECT_EQ(header.point_offset, 1301u);

        EXPECT_EQ(header.vlr_count, 5u);

        EXPECT_EQ(header.point_format_id, 0);
        EXPECT_EQ(header.point_record_length, 20);

        EXPECT_DOUBLE_EQ(header.scale.x, 0.01);
        EXPECT_DOUBLE_EQ(header.scale.y, 0.01);
        EXPECT_DOUBLE_EQ(header.scale.z, 0.01);

        EXPECT_DOUBLE_EQ(header.offset.x, 0.0);
        EXPECT_DOUBLE_EQ(header.offset.y, 0.0);
        EXPECT_DOUBLE_EQ(header.offset.z, 0.0);

        EXPECT_DOUBLE_EQ(header.minimum.x, 493994.87);
        EXPECT_DOUBLE_EQ(header.minimum.y, 4877429.62);
        EXPECT_DOUBLE_EQ(header.minimum.z, 123.93);

        EXPECT_DOUBLE_EQ(header.maximum.x, 494993.68);
        EXPECT_DOUBLE_EQ(header.maximum.y, 4878817.02);
        EXPECT_DOUBLE_EQ(header.maximum.z, 178.73);
/**
        EXPECT_DOUBLE_EQ(header.scale.x, 0.01, 0.0001);
        EXPECT_DOUBLE_EQ(header.scale.y, 0.01, 0.0001);
        EXPECT_DOUBLE_EQ(header.scale.z, 0.01, 0.0001);

        EXPECT_DOUBLE_EQ(header.offset.x, 0.0, 0.0001);
        EXPECT_DOUBLE_EQ(header.offset.y, 0.0, 0.0001);
        EXPECT_DOUBLE_EQ(header.offset.z, 0.0, 0.0001);

        EXPECT_DOUBLE_EQ(header.minimum.x, 493994.87, 0.0001);
        EXPECT_DOUBLE_EQ(header.minimum.y, 4877429.62, 0.0001);
        EXPECT_DOUBLE_EQ(header.minimum.z, 123.93, 0.0001);

        EXPECT_DOUBLE_EQ(header.maximum.x, 494993.68, 0.0001);
        EXPECT_DOUBLE_EQ(header.maximum.y, 4878817.02, 0.0001);
        EXPECT_DOUBLE_EQ(header.maximum.z, 178.73, 0.0001);
**/

        EXPECT_EQ(header.point_count, 1065u);
    }
}

TEST(io_tests, parses_laszip_vlr_correctly) {
    using namespace laszip;

    {
        std::ifstream file(testFile("point10.las.laz"));
        io::reader::file f(file);
        auto vlr = f.get_laz_vlr();

        EXPECT_EQ(vlr.compressor, 2);
        EXPECT_EQ(vlr.coder, 0);

        EXPECT_EQ(vlr.version.major, 2);
        EXPECT_EQ(vlr.version.minor, 2);
        EXPECT_EQ(vlr.version.revision, 0);

        EXPECT_EQ(vlr.options, 0u);
        EXPECT_EQ(vlr.chunk_size, 50000u);

        EXPECT_EQ(vlr.num_points, -1);
        EXPECT_EQ(vlr.num_bytes, -1);

        EXPECT_EQ(vlr.num_items, 1u);
        EXPECT_EQ(vlr.items[0].type, 6);
        EXPECT_EQ(vlr.items[0].size, 20);
        EXPECT_EQ(vlr.items[0].version, 2);
    }
}

void testPoint(laszip::formats::las::point10& p1,
    laszip::formats::las::point10& p2)
{
    EXPECT_EQ(p1.x, p2.x);
    EXPECT_EQ(p1.y, p2.y);
    EXPECT_EQ(p1.z, p2.z);
    EXPECT_EQ(p1.intensity, p2.intensity);
    EXPECT_EQ(p1.return_number, p2.return_number);
    EXPECT_EQ(p1.number_of_returns_of_given_pulse,
        p2.number_of_returns_of_given_pulse);
    EXPECT_EQ(p1.scan_direction_flag, p2.scan_direction_flag);
    EXPECT_EQ(p1.edge_of_flight_line, p2.edge_of_flight_line);
    EXPECT_EQ(p1.classification, p2.classification);
    EXPECT_EQ(p1.scan_angle_rank, p2.scan_angle_rank);
    EXPECT_EQ(p1.user_data, p2.user_data);
    EXPECT_EQ(p1.point_source_ID, p2.point_source_ID);
}

void testPoint(laszip::formats::las::gpstime& p1,
    laszip::formats::las::gpstime& p2)
{
    EXPECT_EQ(p1.value, p2.value);
}

void testPoint(laszip::formats::las::rgb& p1, laszip::formats::las::rgb& p2)
{
    EXPECT_EQ(p1.r, p2.r);
    EXPECT_EQ(p1.g, p2.g);
    EXPECT_EQ(p1.b, p2.b);
}

void testPoint(unsigned char format, unsigned short pointLen,
    char *p1, char *p2)
{
    using namespace laszip;
    using namespace laszip::formats;

    if (format > 4)
        return;

    testPoint(*(las::point10 *)p1, *(las::point10 *)p2);
    p1 += sizeof(las::point10);
    p2 += sizeof(las::point10);
    pointLen -= sizeof(las::point10);
    switch (format)
    {
    case 0:
        break;
    case 1:
        testPoint(*(las::gpstime *)p1, *(las::gpstime *)p2);
        p1 += sizeof(las::gpstime);
        p2 += sizeof(las::gpstime);
        pointLen -= sizeof(las::gpstime);
        break;
    case 2:
        testPoint(*(las::rgb *)p1, *(las::rgb *)p2);
        p1 += sizeof(las::rgb);
        p2 += sizeof(las::rgb);
        pointLen -= sizeof(las::rgb);
        break;
    case 3:
        testPoint(*(las::gpstime *)p1, *(las::gpstime *)p2);
        p1 += sizeof(las::gpstime);
        p2 += sizeof(las::gpstime);
        pointLen -= sizeof(las::gpstime);
        testPoint(*(las::rgb *)p1, *(las::rgb *)p2);
        p1 += sizeof(las::rgb);
        p2 += sizeof(las::rgb);
        pointLen -= sizeof(las::rgb);
        break;
    }

    // Check extra bytes
    while (pointLen--)
        EXPECT_EQ(*p1, *p2);
}

void compare(const std::string& compressed, const std::string& uncompressed)
{
    using namespace laszip;

    std::ifstream cStream(compressed, std::ios::binary);
    std::ifstream ucStream(uncompressed, std::ios::binary);

    io::reader::file c(cStream);
    if (!ucStream.good())
        FAIL() << "Unable to open uncompressed file '" << uncompressed << "'.";

    io::header header;
    ucStream.read((char *)&header, sizeof(io::header));
    ucStream.seekg(header.point_offset);
    unsigned short pointLen = header.point_record_length;

    char ucBuf[1000];
    char cBuf[1000];
    size_t pointCount = c.get_header().point_count;
    for (size_t i = 0; i < pointCount; ++i)
    {
        ucStream.read(ucBuf, pointLen);
        c.readPoint(cBuf);
        testPoint(header.point_format_id, pointLen, ucBuf, cBuf);
    }
}

using SchemaPtr = std::unique_ptr<laszip::factory::record_schema>;

SchemaPtr makeSchema(unsigned char format, size_t record_length)
{
    using namespace laszip;

    SchemaPtr schema(new factory::record_schema);
    (*schema)(factory::record_item::point());
    record_length -= factory::record_item::point().size;
    if (format == 1 || format == 3)
    {
        (*schema)(factory::record_item::gpstime());
        record_length -= factory::record_item::gpstime().size;
    }
    else if (format == 2 || format == 3)
    {
        (*schema)(factory::record_item::rgb());
        record_length -= factory::record_item::rgb().size;
    }
    if (record_length)
        (*schema)(factory::record_item::eb(record_length));
    return schema;
}

void encode(const std::string& lasFilename, const std::string& lazFilename)
{
    using namespace laszip;

    io::header header;
    std::ifstream lasStream(lasFilename, std::ios::binary);
    if (!lasStream.good())
        FAIL() << "Unable to open uncompressed file '" << lasFilename << "'.";

    lasStream.read((char *)&header, sizeof(io::header));
    lasStream.seekg(header.point_offset);

    SchemaPtr schema = makeSchema(header.point_format_id,
        header.point_record_length);

    io::writer::file f(lazFilename, *schema, io::writer::config(header));
    char buf[1000];
    for (size_t i = 0; i < header.point_count; ++i)
    {
        lasStream.read(buf, header.point_record_length);
        f.writePoint(buf);
    }
    f.close();
}

TEST(io_tests, decodes_single_chunk_files_correctly)
{
    compare(testFile("point10.las.laz"), testFile("point10.las"));
}

TEST(io_tests, extrabytes)
{
    compare(testFile("extrabytes.laz"), testFile("extrabytes.las"));
    std::string outLaz(makeTempFileName());
    encode(testFile("extrabytes.las"), outLaz);
    compare(outLaz, testFile("extrabytes.las"));
}

TEST(io_tests, issue44)
{
    // First make sure that we work on the laszip encoded file.
    compare(testFile("1815.laz"), testFile("1815.las"));

    // Encode to a temp laz file and compare.
    std::string outLaz(makeTempFileName());
    encode(testFile("1815.las"), outLaz);
    compare(outLaz, testFile("1815.las"));
}

void checkExists(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f.good()) {
        std::stringstream sstr;
        sstr << "Could not open test file: " << filename << ", did you run the download-test-sets.sh script yet?";
        FAIL() << sstr.str();
    }

    f.close();
}


TEST(io_tests, can_open_large_files) {
    using namespace laszip;
    using namespace laszip::formats;

    checkExists(testFile("autzen_trim.laz"));

    {
        auto func = []() {
            std::ifstream file(testFile("autzen_trim.laz"));
            io::reader::file f(file);
        };

        EXPECT_NO_THROW(func());
    }
}

TEST(io_tests, can_decode_large_files) {
    using namespace laszip;
    using namespace laszip::formats;

    checkExists(testFile("autzen_trim.laz"));
    checkExists(testFile("autzen_trim.las"));

    {
        std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
        io::reader::file f(file);
        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = f.get_header().point_count;

        EXPECT_EQ(pointCount, fin.count_);
        EXPECT_EQ( fin.count_, 110000u);

        struct pnt {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        for (size_t i = 0 ; i < pointCount ; i ++) {
            pnt p1, p2;

            fin.record((char*)&p2);
            f.readPoint((char*)&p1);

            // Make sure the points match
            {
                const las::point10& p = p2.p;
                const las::point10& pout = p1.p;

                EXPECT_EQ(p.x, pout.x);
                EXPECT_EQ(p.y, pout.y);
                EXPECT_EQ(p.z, pout.z);
                EXPECT_EQ(p.intensity, pout.intensity);
                EXPECT_EQ(p.return_number, pout.return_number);
                EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
                EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
                EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
                EXPECT_EQ(p.classification, pout.classification);
                EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
                EXPECT_EQ(p.user_data, pout.user_data);
                EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
            }

            // Make sure the gps time match
            EXPECT_EQ(p1.t.value, p2.t.value);

            // Make sure the colors match
            EXPECT_EQ(p1.c.r, p2.c.r);
            EXPECT_EQ(p1.c.g, p2.c.g);
            EXPECT_EQ(p1.c.b, p2.c.b);
        }
    }
}

TEST(io_tests, can_encode_large_files) {
    using namespace laszip;
    using namespace laszip::formats;

    checkExists(testFile("autzen_trim.laz"));
    checkExists(testFile("autzen_trim.las"));

    // write stuff to a temp file

    {
        // this is the format the autzen has points in
        struct point {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        factory::record_schema schema;

        // make schema
        schema
            (factory::record_item::point())
            (factory::record_item::gpstime())
            (factory::record_item::rgb());

        io::writer::file f(makeTempFileName(), schema,
                io::writer::config({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0}));

        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
        }

        f.close();
    }
}


TEST(io_tests, 1_4) {
    /**
    using namespace laszip;
    using namespace laszip::formats;

    std::string fname = makeTempFileName();

    checkExists(testFile("autzen_trim.las"));
    {
        // this is the format the autzen has points in
        struct point {
            las::point14 p;
        };

        factory::record_schema schema;

        // make schema
        schema
            (factory::record_item::point14());

        io::writer::file f(fname, schema,
            io::writer::config({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0}));

        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
        }

        f.close();
    }
    **/

    /**
    // Now read that back and make sure points match
    {
        // this is the format the autzen has points in
        struct point {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        factory::record_schema schema;

        // make schema
        schema
            (factory::record_item::point())
            (factory::record_item::gpstime())
            (factory::record_item::rgb());

        std::ifstream file(fname, std::ios::binary);
        io::reader::file f(file);
        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p1, p2;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            //std::cout << "# " << i << std::endl;
            fin.record((char*)&p1);
            f.readPoint((char*)&p2);

            // Make sure the points match
            {
                const las::point10& p = p2.p;
                const las::point10& pout = p1.p;

                EXPECT_EQ(p.x, pout.x);
                EXPECT_EQ(p.y, pout.y);
                EXPECT_EQ(p.z, pout.z);
                EXPECT_EQ(p.intensity, pout.intensity);
                EXPECT_EQ(p.return_number, pout.return_number);
                EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
                EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
                EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
                EXPECT_EQ(p.classification, pout.classification);
                EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
                EXPECT_EQ(p.user_data, pout.user_data);
                EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
            }

            // Make sure the gps time match
            EXPECT_EQ(p1.t.value, p2.t.value);

            // Make sure the colors match
            EXPECT_EQ(p1.c.r, p2.c.r);
            EXPECT_EQ(p1.c.g, p2.c.g);
            EXPECT_EQ(p1.c.b, p2.c.b);
        }

        file.close();
    }
    **/
}

///
TEST(io_tests, compression_decompression_is_symmetric) {
    using namespace laszip;
    using namespace laszip::formats;

    std::string fname = makeTempFileName();

    checkExists(testFile("autzen_trim.las"));
    {
        // this is the format the autzen has points in
        struct point {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        factory::record_schema schema;

        // make schema
        schema
            (factory::record_item::point())
            (factory::record_item::gpstime())
            (factory::record_item::rgb());

        io::writer::file f(fname, schema,
            io::writer::config({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0}));

        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
        }

        f.close();
    }

    // Now read that back and make sure points match
    {
        // this is the format the autzen has points in
        struct point {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        factory::record_schema schema;

        // make schema
        schema
            (factory::record_item::point())
            (factory::record_item::gpstime())
            (factory::record_item::rgb());

        std::ifstream file(fname, std::ios::binary);
        io::reader::file f(file);
        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p1, p2;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            //std::cout << "# " << i << std::endl;
            fin.record((char*)&p1);
            f.readPoint((char*)&p2);

            // Make sure the points match
            {
                const las::point10& p = p2.p;
                const las::point10& pout = p1.p;

                EXPECT_EQ(p.x, pout.x);
                EXPECT_EQ(p.y, pout.y);
                EXPECT_EQ(p.z, pout.z);
                EXPECT_EQ(p.intensity, pout.intensity);
                EXPECT_EQ(p.return_number, pout.return_number);
                EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
                EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
                EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
                EXPECT_EQ(p.classification, pout.classification);
                EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
                EXPECT_EQ(p.user_data, pout.user_data);
                EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
            }

            // Make sure the gps time match
            EXPECT_EQ(p1.t.value, p2.t.value);

            // Make sure the colors match
            EXPECT_EQ(p1.c.r, p2.c.r);
            EXPECT_EQ(p1.c.g, p2.c.g);
            EXPECT_EQ(p1.c.b, p2.c.b);
        }

        file.close();
    }
}

TEST(io_tests, can_decode_large_files_from_memory) {
    using namespace laszip;
    using namespace laszip::formats;

    checkExists(testFile("autzen_trim.laz"));
    checkExists(testFile("autzen_trim.las"));

    {
        std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
        EXPECT_EQ(file.good(), true);

        file.ignore((std::numeric_limits<std::streamsize>::max)());
        std::streamsize file_size = file.gcount();
        file.clear();   //  Since ignore will have set eof.
        file.seekg(0, std::ios_base::beg);

        char *buf = (char *)malloc(static_cast<size_t>(file_size));
        file.read(buf, file_size);
        EXPECT_EQ(file.gcount(), file_size);
        file.close();

        memory_stream ms(buf, file_size);

        io::reader::basic_file<memory_stream> f(ms);
        reader fin(testFile("autzen_trim.las"));

        size_t pointCount = f.get_header().point_count;

        EXPECT_EQ(pointCount, fin.count_);

        struct pnt {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        for (size_t i = 0 ; i < pointCount ; i ++) {
            pnt p1, p2;

            f.readPoint((char*)&p1);
            fin.record((char*)&p2);

            // Make sure the points match
            {
                const las::point10& p = p2.p;
                const las::point10& pout = p1.p;

                EXPECT_EQ(p.x, pout.x);
                EXPECT_EQ(p.y, pout.y);
                EXPECT_EQ(p.z, pout.z);
                EXPECT_EQ(p.intensity, pout.intensity);
                EXPECT_EQ(p.return_number, pout.return_number);
                EXPECT_EQ(p.number_of_returns_of_given_pulse, pout.number_of_returns_of_given_pulse);
                EXPECT_EQ(p.scan_direction_flag, pout.scan_direction_flag);
                EXPECT_EQ(p.edge_of_flight_line, pout.edge_of_flight_line);
                EXPECT_EQ(p.classification, pout.classification);
                EXPECT_EQ(p.scan_angle_rank, pout.scan_angle_rank);
                EXPECT_EQ(p.user_data, pout.user_data);
                EXPECT_EQ(p.point_source_ID, pout.point_source_ID);
            }

            // Make sure the gps time match
            EXPECT_EQ(p1.t.value, p2.t.value);

            // Make sure the colors match
            EXPECT_EQ(p1.c.r, p2.c.r);
            EXPECT_EQ(p1.c.g, p2.c.g);
            EXPECT_EQ(p1.c.b, p2.c.b);
        }

        free(buf);
    }
}

TEST(io_tests, writes_bbox_to_header) {
    using namespace laszip;
    using namespace laszip::formats;

    factory::record_schema schema;
    schema(factory::record_item::point());

    // First write a few points
    std::string filename(makeTempFileName());
    io::writer::file f(filename, schema,
        io::writer::config(io::vector3(0.01, 0.01, 0.01), io::vector3(0.0, 0.0, 0.0)));

    las::point10 p1, p2;
    p1.x = 100; p2.x = 200;
    p1.y = -200; p2.y = -300;
    p1.z = 300; p2.z = -400;

    f.writePoint((char*)&p1);
    f.writePoint((char*)&p2);
    f.close();

    // Now check that the file has correct bounding box
    std::ifstream ifs(filename);
    io::reader::file reader(ifs);
    EXPECT_EQ(reader.get_header().minimum.x, 1.0);
    EXPECT_EQ(reader.get_header().maximum.x, 2.0);
    EXPECT_EQ(reader.get_header().minimum.y, -3.0);
    EXPECT_EQ(reader.get_header().maximum.y, -2.0);
    EXPECT_EQ(reader.get_header().minimum.z, -4.0);
    EXPECT_EQ(reader.get_header().maximum.z, 3.0);
}

TEST(io_tests, issue22)
{
    using namespace laszip;
    using namespace laszip::formats;

    std::string infile(testFile("classification.txt"));
    std::string tempfile(testFile("classification.laz"));

    std::ifstream ifs(infile);
    std::istream_iterator<int> it{ifs};
    std::vector<int> cls(it, std::istream_iterator<int>());

    factory::record_schema schema;
    schema(factory::record_item::point());
    io::writer::file out(tempfile, schema,
            io::writer::config({0.01,0.01,0.01}, {0.0,0.0,0.0}));

    las::point10 p10;
    for(const int& cl : cls) {
        p10.classification = static_cast<unsigned char>(cl);
        out.writePoint((char *)&p10);
    }
    out.close();

    std::ifstream instream(tempfile, std::ios::binary);
    io::reader::file in(instream);

    EXPECT_EQ(in.get_header().point_count, cls.size());
    for (size_t i = 0; i < in.get_header().point_count; i++)
    {
        in.readPoint((char *)&p10);
        EXPECT_EQ(cls[i], p10.classification);
    }
    std::remove(tempfile.c_str());
}
