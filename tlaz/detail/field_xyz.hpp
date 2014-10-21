/*
===============================================================================

  FILE:  field_xyz.hpp

  CONTENTS:
	XYZ fields encoder


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

#ifndef __las_hpp__
#error Cannot directly include this file, this is a part of las.hpp
#endif

namespace laszip {
	namespace formats {
		// Teach packers how to pack unpack the xyz struct
		//
		template<>
		struct packers<las::xyz> {
			inline static las::xyz unpack(const char *in) {
				// blind casting will cause problems for ARM and Emscripten targets
				//
				las::xyz p;

				p.x = packers<int>::unpack(in);						in += sizeof(int);
				p.y = packers<int>::unpack(in);						in += sizeof(int);
				p.z = packers<int>::unpack(in);	

				return p;
			}

			inline static void pack(const las::xyz& p, char *buffer) {
				packers<int>::pack(p.x, buffer);					buffer += sizeof(int);
				packers<int>::pack(p.y, buffer);					buffer += sizeof(int);
				packers<int>::pack(p.z, buffer);
			}
		};

		// specialize field to compress point 10
		//
		template<>
		struct field<las::xyz> {
			typedef las::xyz type;

			field() : compressor_inited_(false), decompressors_inited_(false) { }

			template<
				typename TEncoder
			>
			inline void compressWith(TEncoder& enc, const type& this_val) {
				if (!compressor_inited_) {
					compressors_.init();
					compressor_inited_ = true;
				}

				if (!common_.have_last_) {
					// don't have the first data yet, just push it to our have last stuff and move on
					common_.have_last_ = true;
					common_.last_ = this_val;

					// write this out to the encoder as it is
					char buffer[sizeof(las::xyz)];
					packers<type>::pack(this_val, buffer);

					enc.getOutStream().putBytes((unsigned char*)buffer, sizeof(buffer));

					// we are done here
					return;
				}

				unsigned int k_bits;
				int median, diff;
				int n = 1;
				int m = 0;
				int l = 0;

				// compress x coordinate
				median = common_.last_x_diff_median5[m].get();
				diff = this_val.x - common_.last_.x;
				compressors_.ic_dx.compress(enc, median, diff, n == 1);
				common_.last_x_diff_median5[m].add(diff);

				// compress y coordinate
				k_bits = compressors_.ic_dx.getK();
				median = common_.last_y_diff_median5[m].get();
				diff = this_val.y - common_.last_.y;
				compressors_.ic_dy.compress(enc, median, diff, (n==1) + ( k_bits < 20 ? U32_ZERO_BIT_0(k_bits) : 20 ));
				common_.last_y_diff_median5[m].add(diff);

				// compress z coordinate
				k_bits = (compressors_.ic_dx.getK() + compressors_.ic_dy.getK()) / 2;
				compressors_.ic_z.compress(enc, common_.last_height[l], this_val.z, (n==1) + (k_bits < 18 ? U32_ZERO_BIT_0(k_bits) : 18));
				common_.last_height[l] = this_val.z;

				common_.last_ = this_val;
			}

			template<
				typename TDecoder
			>
			inline las::xyz decompressWith(TDecoder& dec) {
				if (!decompressors_inited_) {
					decompressors_.init();
					decompressors_inited_ = true;
				}

				if (!common_.have_last_) {
					// don't have the first data yet, read the whole point out of the stream
					common_.have_last_ = true;

					char buf[sizeof(type)];
					dec.getInStream().getBytes((unsigned char*)buf, sizeof(buf));

					// decode this value
					common_.last_ = packers<type>::unpack(buf);

					// we are done here
					return common_.last_;
				}

				unsigned int k_bits;
				int median, diff;
				int n = 1;
				int m = 0;
				int l = 0;

				// decompress x coordinate
				median = common_.last_x_diff_median5[m].get();

				diff = decompressors_.ic_dx.decompress(dec, median, n==1);
				common_.last_.x += diff;
				common_.last_x_diff_median5[m].add(diff);

				// decompress y coordinate
				median = common_.last_y_diff_median5[m].get();
				k_bits = decompressors_.ic_dx.getK();
				diff = decompressors_.ic_dy.decompress(dec, median, (n==1) + ( k_bits < 20 ? U32_ZERO_BIT_0(k_bits) : 20 ));
				common_.last_.y += diff;
				common_.last_y_diff_median5[m].add(diff);

				// decompress z coordinate
				k_bits = (decompressors_.ic_dx.getK() + decompressors_.ic_dy.getK()) / 2;
				common_.last_.z = decompressors_.ic_z.decompress(dec, common_.last_height[l], (n==1) + (k_bits < 18 ? U32_ZERO_BIT_0(k_bits) : 18));
				common_.last_height[l] = common_.last_.z;

				return common_.last_;
			}

			// All the things we need to compress a point, group them into structs
			// so we don't have too many names flying around

			// Common parts for both a compressor and decompressor go here
			struct __common {
				type last_;

				std::array<utils::streaming_median<int>, 16> last_x_diff_median5;
				std::array<utils::streaming_median<int>, 16> last_y_diff_median5;

				std::array<int, 8> last_height;

				bool have_last_;

				__common() :
					have_last_(false) {
					last_height.fill(0);
				}

				~__common() {
				}
			} common_;

			// These compressors are specific to a compressor usage, so we keep them separate here
			struct __compressors {
				compressors::integer ic_dx;
				compressors::integer ic_dy;
				compressors::integer ic_z;

				__compressors() :
					ic_dx(32, 2),
					ic_dy(32, 22),
					ic_z(32, 20) { }

				void init() {
					ic_dx.init();
					ic_dy.init();
					ic_z.init();
				}
			} compressors_;

			struct __decompressors {
				decompressors::integer ic_dx;
				decompressors::integer ic_dy;
				decompressors::integer ic_z;

				__decompressors() :
					ic_dx(32, 2),
					ic_dy(32, 22),
					ic_z(32, 20) { }

				void init() {
					ic_dx.init();
					ic_dy.init();
					ic_z.init();
				}
			} decompressors_;

			bool compressor_inited_;
			bool decompressors_inited_;
		};
	}
}
