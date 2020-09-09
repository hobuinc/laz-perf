/*
===============================================================================

  FILE:  stream_tests.cpp

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

#include "test_main.hpp"

#include <laz-perf/streams.hpp>

TEST(stream_tests, streams_are_sane) {
	using namespace laszip::streams;

	char buf[1024];

	memory_stream s(buf, sizeof(buf));

	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), false);
	EXPECT_EQ(s.gcount(), 0);

	s.seekg(1024);

	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.eof(), false);
	EXPECT_EQ(s.tellg(), 0);

	s.seekg(512);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), false);
	EXPECT_EQ(s.tellg(), 512);

	char b[64];

	s.read(b, 64);

	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), false);
	EXPECT_EQ(s.tellg(), 512+64);
	EXPECT_EQ(s.gcount(), 64);


	char c[512];
	s.read(c, 512);

	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), true);
	EXPECT_EQ(s.tellg(), 1024);
	EXPECT_EQ(s.gcount(), 512 - 64);

	s.clear();

	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), false);
	EXPECT_EQ(s.tellg(), 1024);

	s.read(c, 64);


	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), true);
	EXPECT_EQ(s.gcount(), 0);
	EXPECT_EQ(s.tellg(), 1024);

	s.seekg(0);

	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.eof(), true); // seek should not clear eof flag
	EXPECT_EQ(s.tellg(), 0);

	s.clear();

	EXPECT_EQ(s.eof(), false);

	s.seekg(2048);

	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.good(), true); // only report last status
	EXPECT_EQ(s.tellg(), 0);

	s.seekg(1024);
	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.good(), true); // only report last status
	EXPECT_EQ(s.tellg(), 0);

	s.seekg(0, std::ios::end);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 1023);

	s.seekg(-10, std::ios::end);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 1013);

	s.seekg(10, std::ios::end);
	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.tellg(), 1013);

	s.seekg(512);
	s.seekg(10, std::ios::cur);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 522);

	s.seekg(1000, std::ios::cur);
	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.tellg(), 522);

	s.seekg(-10, std::ios::cur);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 512);

	s.seekg(0, std::ios::beg);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 0);

	s.seekg(10, std::ios::beg);
	EXPECT_EQ(s.good(), true);
	EXPECT_EQ(s.tellg(), 10);

	s.seekg(0);
	s.seekg(-10, std::ios::beg);
	EXPECT_EQ(s.good(), false);
	EXPECT_EQ(s.tellg(), 0);
}
