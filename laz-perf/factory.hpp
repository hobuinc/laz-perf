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
			enum {
				POINT10 = 6,
				GPSTIME = 7,
				RGB12 = 8
			};

			int type, size, version;
			record_item(int t, int s, int v) :
				type(t), size(s), version(v) {}

			record_item(int t) {
				type = t;
				switch(t) {
					case POINT10: size = 20 ; version = 2; break;
					case GPSTIME: size = 8 ; version = 2; break;
					case RGB12: size = 6 ; version = 2; break;
					default: throw unknown_record_item_type();
				}
			}
		};

		struct record_schema {
			record_schema() : records() { }
			void push(const record_item& item) {
				records.push_back(item);
			}

			record_schema& operator () (const record_item& i) {
				push(i);
				return *this;
			}

			int size_in_bytes() const {
				int sum = 0;
				for (auto i : records) {
					sum += i.size;
				}

				return sum;
			}

			std::vector<record_item> records;
		};

		static inline std::string make_token(const record_schema& s) {
			std::stringstream tokensstr;
			for (auto r : s.records) {
				tokensstr << "v" << r.version << "t" << r.type << "s" << r.size;
			}
			return tokensstr.str();
		}


		template<
			typename TDecoder
		>
		formats::dynamic_decompressor::ptr build_decompressor(TDecoder& dec, const record_schema& schema) {
			// this code may be auto generated in future, please
			using namespace formats;

			std::string hash = make_token(schema);

#define __c(x) (hash.compare(x) == 0)
			if (__c("v2t6s20")) {       // format 0
				// just point 10
				return make_dynamic_decompressor(dec,
						new formats::record_decompressor<field<las::point10> >());
			}
			else if (__c("v2t6s20v2t7s8")) {   // format 1
				// point 10, gpstime
				return make_dynamic_decompressor(dec,
						new formats::record_decompressor<
							field<las::point10>,
							field<las::gpstime> >());
			}
			else if (__c("v2t6s20v2t8s6")) {   // format 2
				// point 10, gpstime
				return make_dynamic_decompressor(dec,
						new formats::record_decompressor<
							field<las::point10>,
							field<las::rgb> >());
			}

			else if (__c("v2t6s20v2t7s8v2t8s6")) {    // format 2
				// point10, gpstime, color
				return make_dynamic_decompressor(dec,
						new formats::record_decompressor<
							field<las::point10>,
							field<las::gpstime>,
							field<las::rgb> >());
			}
#undef __c

			// we got a schema we don't know how to build
			throw unknown_schema_type();
#ifndef _WIN32
			return dynamic_decompressor::ptr(); // avoid warning
#endif
		}

		template<
			typename TEncoder
		>
		formats::dynamic_compressor::ptr build_compressor(TEncoder& enc, const record_schema& schema) {
			using namespace formats;

			std::string hash = make_token(schema);
#define __c(x) (hash.compare(x) == 0)

			if (__c("v2t6s20")) {  // format 0;
				// just point 10
				return make_dynamic_compressor(enc,
						new formats::record_compressor<field<las::point10> >());
			}
			else if (__c("v2t6s20v2t7s8")) { // format 1
				// point10, gpstime
				return make_dynamic_compressor(enc,
						new formats::record_compressor<
							field<las::point10>,
							field<las::gpstime>>());
            }
			else if (__c("v2t6s20v2t8s6")) {    // format 2
				// point10, gpstime, color
				return make_dynamic_compressor(enc,
						new formats::record_compressor<
							field<las::point10>,
							field<las::rgb> >());
			}
			else if (__c("v2t6s20v2t7s8v2t8s6")) { // format 3
				// point10, gpstime, color
				return make_dynamic_compressor(enc,
						new formats::record_compressor<
							field<las::point10>,
							field<las::gpstime>,
							field<las::rgb> >());
			}
#undef __c

			// we got a schema we don't know how to build
			throw unknown_schema_type();
#ifndef _WIN32
			return dynamic_compressor::ptr(); // avoid warning
#endif
		}

		static inline unsigned char schema_to_point_format(const record_schema& schema) {
			// get a point format identifier from the given schema
			std::string hash = make_token(schema);

#define __c(x) (hash.compare(x) == 0)
			if (__c("v2t6s20")) return 0;				// just point
			else if (__c("v2t6s20v2t7s8")) return 1;	// point + gpstime
			else if (__c("v2t6s20v2t8s6")) return 2;	// point + color
			else if (__c("v2t6s20v2t7s8v2t8s6")) return 3; // point + gpstime + color

#undef __c

			throw unknown_schema_type();
			return static_cast<unsigned char>(-1); // avoid warning
		}
	}
}

#endif // __factory_hpp__
