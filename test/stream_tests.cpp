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

#include <boost/test/unit_test.hpp>

#include "streams.hpp"

BOOST_AUTO_TEST_SUITE(lazperf_stream_tests)

BOOST_AUTO_TEST_CASE(streams_are_sane) {
	using namespace laszip::streams;

	char buf[1024];

	memory_stream s(buf, sizeof(buf));

	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), false);
	BOOST_CHECK_EQUAL(s.gcount(), 0);

	s.seekg(1024);

	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.eof(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 0);

	s.seekg(512);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 512);

	char b[64];

	s.read(b, 64);

	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 512+64);
	BOOST_CHECK_EQUAL(s.gcount(), 64);


	char c[512];
	s.read(c, 512);

	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 1024);
	BOOST_CHECK_EQUAL(s.gcount(), 512 - 64);

	s.clear();

	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 1024);

	s.read(c, 64);


	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), true);
	BOOST_CHECK_EQUAL(s.gcount(), 0);
	BOOST_CHECK_EQUAL(s.tellg(), 1024);

	s.seekg(0);

	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.eof(), true); // seek should not clear eof flag
	BOOST_CHECK_EQUAL(s.tellg(), 0);

	s.clear();

	BOOST_CHECK_EQUAL(s.eof(), false);

	s.seekg(2048);

	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.good(), true); // only report last status
	BOOST_CHECK_EQUAL(s.tellg(), 0);

	s.seekg(1024);
	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.good(), true); // only report last status
	BOOST_CHECK_EQUAL(s.tellg(), 0);

	s.seekg(0, std::ios::end);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 1023);

	s.seekg(-10, std::ios::end);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 1013);

	s.seekg(10, std::ios::end);
	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 1013);

	s.seekg(512);
	s.seekg(10, std::ios::cur);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 522);

	s.seekg(1000, std::ios::cur);
	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 522);

	s.seekg(-10, std::ios::cur);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 512);

	s.seekg(0, std::ios::beg);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 0);

	s.seekg(10, std::ios::beg);
	BOOST_CHECK_EQUAL(s.good(), true);
	BOOST_CHECK_EQUAL(s.tellg(), 10);

	s.seekg(0);
	s.seekg(-10, std::ios::beg);
	BOOST_CHECK_EQUAL(s.good(), false);
	BOOST_CHECK_EQUAL(s.tellg(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
