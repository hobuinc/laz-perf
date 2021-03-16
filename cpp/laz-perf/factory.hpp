/*
===============================================================================

  FILE:  factory.hpp
  
  CONTENTS:
    Factory to create dynamic compressors and decompressors

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

#ifndef __factory_hpp__
#define __factory_hpp__

#include "formats.hpp"
#include "excepts.hpp"
#include "las.hpp"

#include <sstream>

namespace lazperf
{
namespace factory
{

las_compressor::ptr build_las_compressor(OutputCb cb, int format, int ebCount = 0)
{
    las_compressor::ptr compressor;

    switch (format)
    {
    case 0:
        compressor.reset(new las::point_compressor_0(cb, ebCount));
        break;
    case 1:
        compressor.reset(new las::point_compressor_1(cb, ebCount));
        break;
    case 2:
        compressor.reset(new las::point_compressor_2(cb, ebCount));
        break;
    case 3:
        compressor.reset(new las::point_compressor_3(cb, ebCount));
        break;
    case 6:
        compressor.reset(new las::point_compressor_6(cb, ebCount));
        break;
    case 7:
        compressor.reset(new las::point_compressor_7(cb, ebCount));
        break;
    case 8:
        compressor.reset(new las::point_compressor_8(cb, ebCount));
    }
    return compressor;
}

las_decompressor::ptr build_las_decompressor(InputCb cb, int format, int ebCount = 0)
{
    las_decompressor::ptr decompressor;

    switch (format)
    {
    case 0:
        decompressor.reset(new las::point_decompressor_0(cb, ebCount));
        break;
    case 1:
        decompressor.reset(new las::point_decompressor_1(cb, ebCount));
        break;
    case 2:
        decompressor.reset(new las::point_decompressor_2(cb, ebCount));
        break;
    case 3:
        decompressor.reset(new las::point_decompressor_3(cb, ebCount));
        break;
    case 6:
        decompressor.reset(new las::point_decompressor_6(cb, ebCount));
        break;
    case 7:
        decompressor.reset(new las::point_decompressor_7(cb, ebCount));
        break;
    case 8:
        decompressor.reset(new las::point_decompressor_8(cb, ebCount));
        break;
    }
    return decompressor;
}

} // namespace factory
} // namespace lazperf

#endif // __factory_hpp__
