/*
===============================================================================

  FILE:  io.hpp

  CONTENTS:
    LAZ io

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

#pragma once

#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>

#include "lazperf_base.hpp"

namespace lazperf
{
namespace io
{
const uint32_t DefaultChunkSize = 50000;
const uint32_t VariableChunkSize = (std::numeric_limits<uint32_t>::max)();

#pragma pack(push, 1)
struct vector3
{
    vector3() : x(0), y(0), z(0)
    {}

    vector3(double x, double y, double z) : x(x), y(y), z(z)
    {}

    double x;
    double y;
    double z;
};

struct base_header
{
    char magic[4] { 'L', 'A', 'S', 'F' };
    uint16_t file_source_id {};
    uint16_t global_encoding {};
    char guid[16] {};

    struct {
        uint8_t major {1};
        uint8_t minor {3};
    } version;

    char system_identifier[32] {};
    char generating_software[32] {};

    struct {
        uint16_t day {};
        uint16_t year {};
    } creation;

    uint16_t header_size {};
    uint32_t point_offset {};
    uint32_t vlr_count {};

    uint8_t point_format_id {};
    uint16_t point_record_length {};

    uint32_t point_count {};
    uint32_t points_by_return[5] {};

    vector3 scale;
    vector3 offset;
    vector3 minimum;
    vector3 maximum;

    int ebCount() const;
    size_t size() const;
};

struct header12 : public base_header
{};

struct header13 : public header12
{
    uint64_t wave_offset {0};
};

struct header14 : public header13
{
    uint64_t evlr_offset {0};
    uint32_t elvr_count {0};
    uint64_t point_count_14 {0};
    uint64_t points_by_return_14[15] {};
};
#pragma pack(pop)
} // namespace io

#define FRIEND_TEST(test_case_name, test_name) \
    friend class test_case_name##_##test_name##_Test

} // namespace lazperf

