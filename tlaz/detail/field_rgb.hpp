// field_rgb.hpp
// RGB related stuff
//

#ifndef __las_hpp__
#error Cannot directly include this file, this is a part of las.hpp
#endif

#define LASZIP_GPSTIME_MULTI 500
#define LASZIP_GPSTIME_MULTI_MINUS -10
#define LASZIP_GPSTIME_MULTI_UNCHANGED (LASZIP_GPSTIME_MULTI - LASZIP_GPSTIME_MULTI_MINUS + 1)
#define LASZIP_GPSTIME_MULTI_CODE_FULL (LASZIP_GPSTIME_MULTI - LASZIP_GPSTIME_MULTI_MINUS + 2)

#define LASZIP_GPSTIME_MULTI_TOTAL (LASZIP_GPSTIME_MULTI - LASZIP_GPSTIME_MULTI_MINUS + 6) 

namespace laszip {
	namespace formats {
		// Teach packers how to pack and unpack rgb
		//
		template<>
		struct packers<las::rgb> {
			inline static las::rgb unpack(const char *in) {
				return las::rgb(packers<unsigned int>::unpack(in),
								packers<unsigned int>::unpack(in+2),
								packers<unsigned int>::unpack(in+4));

			}

			inline static void pack(const las::rgb& c, char *buffer) {
				packers<unsigned short>::pack(c.r, buffer);
				packers<unsigned short>::pack(c.g, buffer+2);
				packers<unsigned short>::pack(c.b, buffer+4);
			}
		};

		namespace detail {
			static inline unsigned int color_diff_bits(const las::rgb& this_val, const las::rgb& last) {

				const las::rgb& a = last,
								b = this_val;

#define __flag_diff(x,y,f) ((((x) ^ (y)) & (f)) != 0)
				unsigned int r =
					(__flag_diff(a.r, b.r, 0x00FF) << 0) |
					(__flag_diff(a.r, b.r, 0xFF00) << 1) |
					(__flag_diff(a.g, b.g, 0x00FF) << 2) |
					(__flag_diff(a.g, b.g, 0xFF00) << 3) |
					(__flag_diff(a.b, b.b, 0x00FF) << 4) |
					(__flag_diff(a.b, b.b, 0xFF00) << 5) |
					(__flag_diff(b.r, b.g, 0x00FF) |
					 __flag_diff(b.r, b.b, 0x00FF) |
					 __flag_diff(b.r, b.g, 0xFF00) |
					 __flag_diff(b.r, b.b, 0xFF00)) << 6;
#undef __flag_diff

				return r;
			}
		}

		// Figure how to compress and decompress GPS time fields
		//
		template<>
		struct field<las::rgb> {
			typedef las::rgb type;

			field():
					have_last_(false),
					last(),
					m_byte_used(128),
					m_rgb_diff_0(256),
					m_rgb_diff_1(256),
					m_rgb_diff_2(256),
					m_rgb_diff_3(256),
					m_rgb_diff_4(256),
					m_rgb_diff_5(256) {}

			template<
				typename TEncoder
			>
			inline void compressWith(TEncoder& enc, const las::rgb& this_val) {
				if (!have_last_) {
					// don't have the first data yet, just push it to our have last stuff and move on
					have_last_ = true;
					last = this_val;

					// write this out to the encoder as it is
					char buffer[sizeof(las::gpstime)];
					packers<las::rgb>::pack(this_val, buffer);

					enc.getOutStream().putBytes((unsigned char*)buffer, sizeof(buffer));

					// we are done here
					return;
				}

				// compress color
				int diff_l = 0;
				int diff_h = 0;
				int corr;

				unsigned int sym = detail::color_diff_bits(this_val, last);

				enc.encodeSymbol(m_byte_used, sym);

				// high and low R
				if (sym & (1 << 0)) {
					diff_l = (this_val.r & 0xFF) - (last.r & 0xFF);
					enc.encodeSymbol(m_rgb_diff_0, U8_FOLD(diff_l));
				}
				if (sym & (1 << 1)) {
					diff_h = static_cast<int>(this_val.r >> 8) - (last.r >> 8);
					enc.encodeSymbol(m_rgb_diff_1, U8_FOLD(diff_h));
				}

				if (sym & (1 << 6)) {
					if (sym & (1 << 2)) {
						corr = static_cast<int>(this_val.g & 0xFF) - U8_CLAMP(diff_l + (last.g & 0xFF));
						enc.encodeSymbol(m_rgb_diff_2, U8_FOLD(corr));
					}

					if (sym & (1 << 4)) {
						diff_l = (diff_l + (this_val.g & 0xFF) - (last.g & 0xFF)) / 2;
						corr = static_cast<int>(this_val.b & 0xFF) - U8_CLAMP(diff_l + (last.b & 0xFF));
						enc.encodeSymbol(m_rgb_diff_4, U8_FOLD(corr));
					}

					if (sym & (1 << 3)) {
						corr = static_cast<int>(this_val.g >> 8) - U8_CLAMP(diff_h + (last.g >> 8));
						enc.encodeSymbol(m_rgb_diff_3, U8_FOLD(corr));
					}

					if (sym & (1 << 5)) {
						diff_h = (diff_h + ((this_val.g >> 8)) - (last.g >> 8)) / 2;
						corr = static_cast<int>(this_val.b >> 8) - U8_CLAMP(diff_h + (last.b >> 8));
						enc.encodeSymbol(m_rgb_diff_5, U8_FOLD(corr));
					}
				}

				last = this_val;
			}
			template<
				typename TDecoder
			>
			inline las::rgb decompressWith(TDecoder& dec) {
				if (!have_last_) {
					// don't have the first data yet, read the whole point out of the stream
					have_last_ = true;

					char buf[sizeof(las::gpstime)];
					dec.getInStream().getBytes((unsigned char*)buf, sizeof(buf));

					// decode this value
					last = packers<las::rgb>::unpack(buf);

					// we are done here
					return last;
				}
				
				unsigned char corr;
				int diff = 0;
				unsigned int sym = dec.decodeSymbol(m_byte_used);

				las::rgb this_val;

				if (sym & (1 << 0)) {
					corr = dec.decodeSymbol(m_rgb_diff_0);
					this_val.r = static_cast<unsigned short>(U8_FOLD(corr + (last.r & 0xFF)));
				}
				else {
					this_val.r = last.r & 0xFF;
				}

				if (sym & (1 << 1)) {
					corr = dec.decodeSymbol(m_rgb_diff_1);
					this_val.r |= (static_cast<unsigned short>(U8_FOLD(corr + (last.r >> 8))) << 8);
				}
				else {
					this_val.r |= last.r & 0xFF00;
				}

				if (sym & (1 << 6)) {
					diff = (this_val.r & 0xFF) - (last.r & 0xFF);

					if (sym & (1 << 2)) {
						corr = dec.decodeSymbol(m_rgb_diff_2);
						this_val.g = static_cast<unsigned short>(U8_FOLD(corr + U8_CLAMP(diff + (last.g & 0xFF))));
					}
					else {
						this_val.g = last.g & 0xFF;
					}

					if (sym & (1 << 4)) {
						corr = dec.decodeSymbol(m_rgb_diff_4);
						diff = (diff + (this_val.g & 0xFF) - (last.g & 0xFF)) / 2;
						this_val.b = static_cast<unsigned short>(U8_FOLD(corr + U8_CLAMP(diff+(last.b & 0xFF))));
					}
					else {
						this_val.b = last.b & 0xFF;
					}


					diff = (this_val.r >> 8) - (last.r >> 8);
					if (sym & (1 << 3)) {
						corr = dec.decodeSymbol(m_rgb_diff_3);
						this_val.g |= static_cast<unsigned short>(U8_FOLD(corr + U8_CLAMP(diff + (last.g >> 8)))) << 8;
					}
					else {
						this_val.g |= last.g & 0xFF00;
					}


					if (sym & (1 << 5)) {
						corr = dec.decodeSymbol(m_rgb_diff_5);
						diff = (diff + (this_val.g >> 8) - (last.g >> 8)) / 2;

						this_val.b |= static_cast<unsigned short>(U8_FOLD(corr + U8_CLAMP(diff + (last.b >> 8)))) << 8;
					}
					else {
						this_val.b |= (last.b & 0xFF00);
					}
				}
				else
				{
					this_val.g = this_val.r;
					this_val.b = this_val.r;
				}

				last = this_val;
				return this_val;
			}

			// All the things we need to compress a point, group them into structs
			// so we don't have too many names flying around

			// Common parts for both a compressor and decompressor go here
			bool have_last_;
			las::rgb last;

			models::arithmetic m_byte_used;
			models::arithmetic m_rgb_diff_0,
							   m_rgb_diff_1,
							   m_rgb_diff_2,
							   m_rgb_diff_3,
							   m_rgb_diff_4,
							   m_rgb_diff_5;
		};
	}
}
