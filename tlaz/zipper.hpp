// zipper.hpp
// Templatization of things
//


#ifndef __zipper_hpp__
#define __zipper_hpp__

#include "../common/types.hpp"

namespace laszip {
	namespace types {
		struct point10 {
			I32 x;
			I32 y;
			I32 z;
			U16 intensity;
			U8 return_number : 3;
			U8 number_of_returns_of_given_pulse : 3;
			U8 scan_direction_flag : 1;
			U8 edge_of_flight_line : 1;
			U8 classification;
			I8 scan_angle_rank;
			U8 user_data;
			U16 point_source_ID;
		}
	}

	namespace compressors {
		struct point10 {
		};
	}

	template<typename TPointType, typename TOut>
	class zipper {
	public:
		zipper(TOut& out) : out_(out) {
		}

		void write(const TPointType& p) {
		}

	private:
		TOut& out_;
	}
}

#endif
