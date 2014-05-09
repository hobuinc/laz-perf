// las.hpp
// Point formats for LAS
//


#ifndef __las_hpp__
#define __las_hpp__

#include "formats.hpp"
#include "model.hpp"
#include "compressor.hpp"
#include "util.hpp"

namespace laszip {
	namespace formats {
		namespace las {
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
		}
	}
}

#include "detail/field_point10.hpp"
#include "detail/field_gpstime.hpp"
#include "detail/field_rgb.hpp"

#endif // __las_hpp__
