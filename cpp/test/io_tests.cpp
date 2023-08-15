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
    terms of the Apache Public License 2.0 published by the Apache Software
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
#include <random>

#include "test_main.hpp"

#include <lazperf/excepts.hpp>
#include <lazperf/las.hpp>
#include <lazperf/readers.hpp>
#include <lazperf/writers.hpp>
#include <lazperf/vlr.hpp>

#include <cstdio>
#include "reader.hpp"
#include <stdio.h>

namespace lazperf
{

std::string makeTempFileName()
{
    std::string filename;
#ifdef _WIN32
    const char *fnTemplate = "fnXXXXXX";
    char name[9];
    strncpy(name, fnTemplate, 9);
    _mktemp_s(name, 9);
    char path[MAX_PATH];
    GetTempPath(MAX_PATH, path);
    filename = std::string(path) + name;
#else
    std::string s("/var/tmp/fnXXXXXX");
    mkstemp(const_cast<char *>(s.data()));
    filename = s;
#endif
    return filename;
}

TEST(io_tests, io_structs_are_of_correct_size)
{
    EXPECT_EQ(header12::Size, 227u);
    EXPECT_EQ(header13::Size, 235u);
    EXPECT_EQ(header14::Size, 375u);
}

TEST(io_tests, can_report_invalid_magic)
{
    auto func = []() {
        reader::named_file f(testFile("point10-1.las.raw"));
    };
    EXPECT_THROW(func(), error);
}

TEST(io_tests, doesnt_throw_any_errors_for_valid_laz)
{
    auto func = []() {
        reader::named_file f(testFile("point10.las.laz"));
   };

   EXPECT_NO_THROW(func());
}

void dumpBytes(const char* b, size_t len) {
    for (size_t i = 0 ; i < len ; i ++) {
        printf("%02X ", (unsigned char)b[i]);

        if ((i+1) % 16 == 0)
            printf("\n");
    }
}


TEST(io_tests, parses_header_correctly)
{
    reader::named_file f(testFile("point10.las.laz"));
    const base_header& header = f.header();

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

    EXPECT_DOUBLE_EQ(header.minx, 493994.87);
    EXPECT_DOUBLE_EQ(header.miny, 4877429.62);
    EXPECT_DOUBLE_EQ(header.minz, 123.93);

    EXPECT_DOUBLE_EQ(header.maxx, 494993.68);
    EXPECT_DOUBLE_EQ(header.maxy, 4878817.02);
    EXPECT_DOUBLE_EQ(header.maxz, 178.73);

    EXPECT_EQ(header.point_count, 1065u);
}

TEST(io_tests, parses_laszip_vlr_correctly)
{
    reader::named_file f(testFile("point10.las.laz"));
    laz_vlr vlr = f.lazVlr();

    auto check = [](const laz_vlr& vlr)
    {
        EXPECT_EQ(vlr.compressor, 2);
        EXPECT_EQ(vlr.coder, 0);

        EXPECT_EQ(vlr.ver_major, 2);
        EXPECT_EQ(vlr.ver_minor, 2);
        EXPECT_EQ(vlr.revision, 0);

        EXPECT_EQ(vlr.options, 0u);
        EXPECT_EQ(vlr.chunk_size, 50000u);

        EXPECT_EQ(vlr.num_points, -1);
        EXPECT_EQ(vlr.num_bytes, -1);

        EXPECT_EQ(vlr.items.size(), 1u);
        EXPECT_EQ(vlr.items[0].type, 6);
        EXPECT_EQ(vlr.items[0].size, 20);
        EXPECT_EQ(vlr.items[0].version, 2);
    };

    // Test deprecated functions.
    check(vlr);
    std::vector<char> vd = vlr.data();
    laz_vlr vlr2(vd.data());
    check(vlr2);
}

void testPoint(const lazperf::las::point10& p1, const lazperf::las::point10& p2)
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

void testPoint(const lazperf::las::gpstime& p1, const lazperf::las::gpstime& p2)
{
    EXPECT_EQ(p1.value, p2.value);
}

void testPoint(const lazperf::las::rgb& p1, const lazperf::las::rgb& p2)
{
    EXPECT_EQ(p1.r, p2.r);
    EXPECT_EQ(p1.g, p2.g);
    EXPECT_EQ(p1.b, p2.b);
}

void testPoint(unsigned char format, unsigned short pointLen, const char *p1, const char *p2)
{

    if (format > 4)
        return;

    testPoint(*(const las::point10 *)p1, *(const las::point10 *)p2);
    p1 += sizeof(las::point10);
    p2 += sizeof(las::point10);
    pointLen -= sizeof(las::point10);
    switch (format)
    {
    case 0:
        break;
    case 1:
        testPoint(*(const las::gpstime *)p1, *(const las::gpstime *)p2);
        p1 += sizeof(las::gpstime);
        p2 += sizeof(las::gpstime);
        pointLen -= sizeof(las::gpstime);
        break;
    case 2:
        testPoint(*(const las::rgb *)p1, *(const las::rgb *)p2);
        p1 += sizeof(las::rgb);
        p2 += sizeof(las::rgb);
        pointLen -= sizeof(las::rgb);
        break;
    case 3:
        testPoint(*(const las::gpstime *)p1, *(const las::gpstime *)p2);
        p1 += sizeof(las::gpstime);
        p2 += sizeof(las::gpstime);
        pointLen -= sizeof(las::gpstime);
        testPoint(*(const las::rgb *)p1, *(const las::rgb *)p2);
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
    reader::named_file c(compressed);

    std::ifstream ucStream(uncompressed, std::ios::binary);
    if (!ucStream.good())
        FAIL() << "Unable to open uncompressed file '" << uncompressed << "'.";


    header12 header = header12::create(ucStream);
    ucStream.seekg(header.point_offset);
    unsigned short pointLen = header.point_record_length;

    char ucBuf[1000];
    char cBuf[1000];
    size_t pointCount = c.header().point_count;
    for (size_t i = 0; i < pointCount; ++i)
    {
        ucStream.read(ucBuf, pointLen);
        c.readPoint(cBuf);
        testPoint(header.point_format_id, pointLen, ucBuf, cBuf);
    }
}

void encode(const std::string& lasFilename, const std::string& lazFilename)
{
    std::ifstream lasStream(lasFilename, std::ios::binary);
    if (!lasStream.good())
        FAIL() << "Unable to open uncompressed file '" << lasFilename << "'.";

    header12 h = header12::create(lasStream);
    lasStream.seekg(h.point_offset);

    writer::named_file f(lazFilename, writer::named_file::config(h));
    std::vector<char> buf(1000);
    for (size_t i = 0; i < h.point_count; ++i)
    {
        lasStream.read(buf.data(), h.point_record_length);
        f.writePoint(buf.data());
    }
    f.close();
}

TEST(io_tests, decodes_single_chunk_files_correctly)
{
    compare(testFile("point10.las.laz"), testFile("point10.las"));
}

TEST(io_tests, extrabytes)
{
    // Total point length of 61 - 27 extra bytes
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


TEST(io_tests, can_open_large_files)
{
    checkExists(testFile("autzen_trim.laz"));

    {
        auto func = []() {
            reader::named_file f(testFile("autzen_trim.laz"));
        };

        EXPECT_NO_THROW(func());
    }
}

TEST(io_tests, can_decode_large_files)
{
    checkExists(testFile("autzen_trim.laz"));
    checkExists(testFile("autzen_trim.las"));

    {
        std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
        reader::named_file f(testFile("autzen_trim.laz"));
        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = f.header().point_count;

        EXPECT_EQ(pointCount, fin.count_);
        EXPECT_EQ(fin.count_, 110000u);

        struct pnt {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        for (size_t i = 0; i < pointCount; i++)
        {
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

TEST(io_tests, can_encode_large_files)
{
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

        writer::named_file::config c({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0});
        c.pdrf = 3;
        c.chunk_size = 0;
        writer::named_file f(makeTempFileName(), c);

        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0 ; i < pointCount ; i ++) {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
        }

        f.close();
    }
}


TEST(io_tests, variable_chunks)
{
    std::string fname = makeTempFileName();

    // this is the format the autzen has points in
    struct point {
        las::point10 p;
        las::gpstime t;
        las::rgb c;
    };

    checkExists(testFile("autzen_trim.las"));
    {

        writer::named_file::config c({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0}, VariableChunkSize);
        c.pdrf = 3;
        writer::named_file f(fname, c);

        test::reader fin(testFile("autzen_trim.las"));

        std::mt19937 rd(12235234);
        std::uniform_int_distribution<uint32_t> dist(500, 10000);

        size_t chunksize = dist(rd);
        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0; i < pointCount; i++)
        {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
            chunksize--;
            if (chunksize == 0)
            {
                f.newChunk();
                chunksize = dist(rd);
            }
        }
        f.close();
    }

    // Now read that back and make sure points match
    {
        reader::named_file f(fname);
        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;

        for (size_t i = 0; i < pointCount; ++i)
        {
            point p1, p2;

            f.readPoint((char*)&p1);
            fin.record((char*)&p2);
            testPoint(3, sizeof(p1), (const char *)&p1, (const char *)&p2);
        }
    }
}


TEST(io_tests, compression_decompression_is_symmetric)
{
    std::string fname = makeTempFileName();

    // this is the format the autzen has points in
    struct point {
        las::point10 p;
        las::gpstime t;
        las::rgb c;
    };

    checkExists(testFile("autzen_trim.las"));
    {

        writer::named_file::config c({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0});
        c.pdrf = 3;
        writer::named_file f(fname, c);

        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;
        point p;
        for (size_t i = 0; i < pointCount; i++)
        {
            fin.record((char*)&p);
            f.writePoint((char*)&p);
        }

        f.close();
    }

    // Now read that back and make sure points match
    {
        reader::named_file f(fname);
        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = fin.count_;

        for (size_t i = 0; i < pointCount; ++i)
        {
            point p1, p2;

            f.readPoint((char*)&p1);
            fin.record((char*)&p2);
            testPoint(3, sizeof(p1), (const char *)&p1, (const char *)&p2);
        }
    }
}

TEST(io_tests, can_decode_large_files_from_memory)
{
    checkExists(testFile("autzen_trim.laz"));
    checkExists(testFile("autzen_trim.las"));

    {
        std::ifstream file(testFile("autzen_trim.laz"), std::ios::binary);
        EXPECT_EQ(file.good(), true);

        file.ignore((std::numeric_limits<std::streamsize>::max)());
        std::streamsize file_size = file.gcount();
        file.clear();   //  Since ignore will have set eof.
        file.seekg(0, std::ios_base::beg);

        std::vector<char> buf(file_size);
        file.read(buf.data(), file_size);
        EXPECT_EQ(file.gcount(), file_size);
        file.close();

        reader::mem_file f(buf.data(), file_size);
        test::reader fin(testFile("autzen_trim.las"));

        size_t pointCount = f.header().point_count;

        EXPECT_EQ(pointCount, fin.count_);

        struct pnt {
            las::point10 p;
            las::gpstime t;
            las::rgb c;
        };

        for (size_t i = 0 ; i < pointCount ; i ++)
        {
            pnt p1, p2;

            f.readPoint((char*)&p1);
            fin.record((char*)&p2);
            testPoint(3, sizeof(p1), (const char *)&p1, (const char *)&p2);
        }
    }
}

TEST(io_tests, writes_bbox_to_header)
{
    // First write a few points
    std::string filename(makeTempFileName());
    writer::named_file::config c({0.01, 0.01, 0.01}, {0.0, 0.0, 0.0});
    c.pdrf = 0;
    writer::named_file f(filename, c);

    las::point10 p1, p2;
    p1.x = 100; p2.x = 200;
    p1.y = -200; p2.y = -300;
    p1.z = 300; p2.z = -400;

    f.writePoint((char*)&p1);
    f.writePoint((char*)&p2);
    f.close();

    // Now check that the file has correct bounding box
    reader::named_file reader(filename);
    EXPECT_EQ(reader.header().minx, 1.0);
    EXPECT_EQ(reader.header().maxx, 2.0);
    EXPECT_EQ(reader.header().miny, -3.0);
    EXPECT_EQ(reader.header().maxy, -2.0);
    EXPECT_EQ(reader.header().minz, -4.0);
    EXPECT_EQ(reader.header().maxz, 3.0);
}

TEST(io_tests, issue22)
{
    std::string infile(testFile("classification.txt"));
    std::string tempfile(testFile("classification.laz"));

    std::ifstream ifs(infile);
    std::istream_iterator<int> it{ifs};
    std::vector<int> cls(it, std::istream_iterator<int>());

    writer::named_file::config c({0.01,0.01,0.01}, {0.0,0.0,0.0});
    writer::named_file out(tempfile, c);

    las::point10 p10;
    for(const int& cl : cls) {
        p10.classification = static_cast<unsigned char>(cl);
        out.writePoint((char *)&p10);
    }
    out.close();

    reader::named_file in(tempfile);

    EXPECT_EQ(in.header().point_count, cls.size());
    for (size_t i = 0; i < in.header().point_count; i++)
    {
        in.readPoint((char *)&p10);
        EXPECT_EQ(cls[i], p10.classification);
    }
    std::remove(tempfile.c_str());
}

TEST(io_tests, vlrIndex)
{
    reader::named_file f(testFile("point10.las.laz"));

    std::vector<char> vlr = f.vlrData("LASF_Projection", 34735);
    EXPECT_EQ(vlr.size(), 72u);
    vlr = f.vlrData("Foo", 34735);
    EXPECT_EQ(vlr.size(), 0u);
    vlr = f.vlrData("LASF_Projection", 25);
    EXPECT_EQ(vlr.size(), 0u);
    vlr = f.vlrData("LASF_Projection", 34737);
    EXPECT_EQ(vlr.size(), 28u);
    vlr = f.vlrData("liblas", 2112);
    EXPECT_EQ(vlr.size(), 640u);
}

/**
TEST(io_tests, fast)
{
    std::string dir = "/Users/acbell/nyc2/";
    std::string las = dir + "1.las";
    std::string laz = dir + "1.laz";
    compare(laz, las);
    std::string outLaz(makeTempFileName());
    for (int i = 0; i < 300; ++i)
        encode(las, outLaz);
    compare(outLaz, las);
}
**/

namespace
{

void pre_1_2_common_header_checks(const base_header& header)
{
  EXPECT_EQ(header.global_encoding, 0);
  EXPECT_EQ(header.version.major, 1);
  EXPECT_EQ(header.creation.day, 200);
  EXPECT_EQ(header.creation.year, 2023);
  EXPECT_EQ(header.header_size, 227);
  EXPECT_EQ(header.point_format_id, 0);
  EXPECT_EQ(header.point_record_length, 20);
  EXPECT_DOUBLE_EQ(header.scale.x, 1.0);
  EXPECT_DOUBLE_EQ(header.scale.y, 1.0);
  EXPECT_DOUBLE_EQ(header.scale.z, 1.0);
  EXPECT_DOUBLE_EQ(header.offset.x, 0.0);
  EXPECT_DOUBLE_EQ(header.offset.y, 0.0);
  EXPECT_DOUBLE_EQ(header.offset.z, 0.0);
  EXPECT_DOUBLE_EQ(header.minx, 1.0);
  EXPECT_DOUBLE_EQ(header.miny, 2.0);
  EXPECT_DOUBLE_EQ(header.minz, 3.0);
  EXPECT_DOUBLE_EQ(header.maxx, 1.0);
  EXPECT_DOUBLE_EQ(header.maxy, 2.0);
  EXPECT_DOUBLE_EQ(header.maxz, 3.0);
  EXPECT_EQ(header.point_count, 1);
}

void pre_1_2_common_point_checks(const las::point10& p10)
{
  EXPECT_DOUBLE_EQ(p10.x, 1.0);
  EXPECT_DOUBLE_EQ(p10.y, 2.0);
  EXPECT_DOUBLE_EQ(p10.z, 3.0);
  EXPECT_EQ(p10.intensity, 42);
  EXPECT_EQ(p10.return_number, 1);
  EXPECT_EQ(p10.number_of_returns_of_given_pulse, 1);
  EXPECT_EQ(p10.classification, 5);
  EXPECT_EQ(p10.scan_angle_rank, 7);
}

}

TEST(io_tests, can_read_1_0)
{
  for (const std::string filename : { "point10-1.0.las", "point10-1.0.laz" })
  {
    reader::named_file f(testFile(filename));
    const base_header& header = f.header();
    pre_1_2_common_header_checks(header);
    EXPECT_EQ(header.version.minor, 0);
    EXPECT_EQ(header.global_encoding, 0);

    las::point10 p10;
    f.readPoint(reinterpret_cast<char*>(&p10));
    pre_1_2_common_point_checks(p10);
    EXPECT_EQ(p10.user_data, 0);
  }
}

TEST(io_tests, can_read_1_1)
{
  for (const std::string filename : { "point10-1.1.las", "point10-1.1.laz" })
  {
    reader::named_file f(testFile(filename));
    const base_header& header = f.header();
    pre_1_2_common_header_checks(header);
    EXPECT_EQ(header.version.minor, 1);
    EXPECT_EQ(header.file_source_id, 1000);

    las::point10 p10;
    f.readPoint(reinterpret_cast<char*>(&p10));
    pre_1_2_common_point_checks(p10);
    EXPECT_EQ(p10.user_data, 13);
  }
}

} // namespace lazperf
