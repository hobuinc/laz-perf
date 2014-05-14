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

namespace laszip {
	namespace factory {
		struct record_item {
			int type, size, version;
			
			record_item(int t, int s, int v) :
				type(t), size(s), version(v) {}
		};

		struct record_schema {
			record_schema() : records() { }
			void push(const record_item& item) {
				records.push_back(item);
			}

			std::vector<record_item> records;
		};


		template<
			typename TEncoder
		>
		formats::dynamic_decompressor::ptr build_decompressor(TEncoder& enc, const record_schema& schema) {
			// this code may be auto generated in future, please
			using namespace formats;

			std::stringstream tokensstr;

			for (auto r : schema.records) {
				tokensstr << "v" << r.version << "t" << r.type << "s" << r.size;
			}

			std::string hash = tokensstr.str();
			if (hash == "v2t6s20") {
				// just point 10
				return make_dynamic_decompressor(enc,
						new formats::record_decompressor<field<las::point10> >());
			}
			else if (hash == "v2t6s20v2t7s8v2t8s6") {
				// point10, gpstime, color
				return make_dynamic_decompressor(enc,
						new formats::record_decompressor<
							field<las::point10>,
							field<las::gpstime>,
							field<las::rgb> >());
			}

			std::cout << "Schema is " << hash << std::endl;


			// we got a schema we don't know how to build
			throw unknown_schema_type();
			return dynamic_decompressor::ptr(); // avoid warning
		}
	}
};

#endif // __factory_hpp__
