/*
===============================================================================

  FILE:  formats.hpp
  
  CONTENTS:
    Format support

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
#ifndef __formats_hpp__
#define __formats_hpp__

#include <iostream>
#include "compressor.hpp"
#include "decompressor.hpp"

namespace laszip {
	namespace formats {
		template<typename T>
		struct packers {
			static_assert(sizeof(T) == 0,
					"Only specialized instances of packers should be used");
		};

		template<>
		struct packers<unsigned int> {
			static unsigned int unpack(const char *in) {
				unsigned int b1 = in[0],
							 b2 = in[1],
							 b3 = in[2],
							 b4 = in[3];

				return (b4 << 24) |
					((b3 & 0xFF) << 16) |
					((b2 & 0xFF) << 8) |
					(b1 & 0xFF);
			}

			static void pack(const unsigned int& v, char *out) {
				out[3] = (v >> 24) & 0xFF;
				out[2] = (v >> 16) & 0xFF;
				out[1] = (v >> 8) & 0xFF;
				out[0] = v & 0xFF;
			}
		};

		template<>
		struct packers<unsigned short> {
			static unsigned short unpack(const char *in) {
				unsigned short b1 = in[0],
							 b2 = in[1];

				return (((b2 & 0xFF) << 8) | (b1 & 0xFF));
			}

			static void pack(const unsigned short& v, char *out) {
				out[1] = (v >> 8) & 0xFF;
				out[0] = v & 0xFF;
			}
		};

		template<>
		struct packers<unsigned char> {
			static unsigned char unpack(const char *in) {
				return static_cast<unsigned char>(in[0]);
			}

			static void pack(const unsigned char& c, char *out) {
				out[0] = static_cast<char>(c);
			}
		};

		template<>
		struct packers<int> {
			static int unpack(const char *in) {
				return static_cast<int>(packers<unsigned int>::unpack(in));
			}

			static void pack(const int& t, char *out) {
				packers<unsigned int>::pack(static_cast<unsigned int>(t), out);
			}
		};

		template<>
		struct packers<short> {
			static short unpack(const char *in) {
				return static_cast<short>(packers<unsigned short>::unpack(in));
			}

			static void pack(const short& t, char *out) {
				packers<unsigned short>::pack(static_cast<unsigned short>(t), out);
			}
		};

		template<>
		struct packers<char> {
			static char unpack(const char *in) {
				return in[0];
			}

			static void pack(const char& t, char *out) {
				out[0] = t;
			}
		};

		/** A simple strategy which returns simple diffs */
		template<typename T>
		struct standard_diff_method {
			standard_diff_method<T>()
				: have_value_(false) {}

			inline void push(const T& v) {
				if (!have_value_)
					have_value_ = true;

				value = v;
			}

			inline bool have_value() const {
				return have_value_;
			}

			T value;
			bool have_value_;
		};

		template<typename T, typename TDiffMethod = standard_diff_method<T> >
		struct field {
			static_assert(std::is_integral<T>::value,
					"Default implementation for field only handles integral types");

			typedef T type;

			field() :
				compressor_(sizeof(T) * 8),
				decompressor_(sizeof(T) * 8),
				compressor_inited_(false),
				decompressor_inited_(false) { }

			template<
				typename TEncoder
			>
			inline void compressWith(TEncoder& encoder, const T& this_val) {
				if (!compressor_inited_)
					compressor_.init();

				// Let the differ decide what values we're going to push
				//
				if (differ_.have_value()) {
					compressor_.compress(encoder, differ_.value, this_val, 0);
				}
				else {
					// differ is not ready for us to start encoding values for us, so we need to write raw into
					// the outputstream
					//
					char buf[sizeof(T)];
					packers<T>::pack(this_val, buf);

					encoder.getOutStream().putBytes((unsigned char*)buf, sizeof(T));
				}

				differ_.push(this_val);
			}

			template<
				typename TDecoder
			>
			inline T decompressWith(TDecoder& decoder) {
				if (!decompressor_inited_)
					decompressor_.init();

				T r;
				if (differ_.have_value()) {
					r = decompressor_.decompress(decoder, differ_.value, 0);
				}
				else {
					// this is probably the first time we're reading stuff, read the record as is
					char buffer[sizeof(T)];
					decoder.getInStream().getBytes((unsigned char*)buffer, sizeof(T));

					r = packers<T>::unpack(buffer);
				}

				differ_.push(r);
				return r;
			}

			laszip::compressors::integer compressor_;
			laszip::decompressors::integer decompressor_;

			bool compressor_inited_, decompressor_inited_;

			TDiffMethod differ_;
		};

		template<typename... TS>
		struct record_compressor;

		template<typename... TS>
		struct record_decompressor;

		template<>
		struct record_compressor<> {
			record_compressor() {}
			template<
				typename T
			>
			inline void compressWith(T&, const char *) { }
		};

		template<typename T, typename... TS>
		struct record_compressor<T, TS...> {
			record_compressor() {}

			template<
				typename TEncoder
			>
			inline void compressWith(TEncoder& encoder, const char *buffer) {
				typedef typename T::type this_field_type;

				this_field_type v = packers<this_field_type>::unpack(buffer);
				field_.compressWith(encoder, v);

				// Move on to the next field
				next_.compressWith(encoder, buffer + sizeof(typename T::type));
			}

			// The field that we handle
			T field_;

			// Our default strategy right now is to just encode diffs, but we would employ more advanced techniques soon
			record_compressor<TS...> next_;
		};

		template<>
		struct record_decompressor<> {
			record_decompressor() : firstDecompress(true) {}
			template<
				typename TDecoder
			>
			inline void decompressWith(TDecoder& decoder, char *) {
				if (firstDecompress) {
					decoder.readInitBytes();
					firstDecompress = false;
				}
			}

			bool firstDecompress;
		};

		template<typename T, typename... TS>
		struct record_decompressor<T, TS...> {
			record_decompressor() {}

			template<
				typename TDecoder
			>
			inline void decompressWith(TDecoder& decoder, char *buffer) {
				typedef typename T::type this_field_type;

				this_field_type v = field_.decompressWith(decoder);
				packers<this_field_type>::pack(v, buffer);

				// Move on to the next field
				next_.decompressWith(decoder, buffer + sizeof(typename T::type));
			}

			// The field that we handle
			T field_;

			// Our default strategy right now is to just encode diffs, but we would employ more advanced techniques soon
			record_decompressor<TS...> next_;
		};

		struct dynamic_compressor {
			typedef std::shared_ptr<dynamic_compressor> ptr;

			virtual void compress(const char *in) = 0;
			virtual ~dynamic_compressor() {}
		};

		struct dynamic_decompressor {
			typedef std::shared_ptr<dynamic_decompressor> ptr;

			virtual void decompress(char *in) = 0;
			virtual ~dynamic_decompressor() {}
		};

		template<
			typename TEncoder,
			typename TRecordCompressor
		>
		struct dynamic_compressor1 : public dynamic_compressor {
			dynamic_compressor1(TEncoder& enc, TRecordCompressor* compressor) :
				enc_(enc), compressor_(compressor) {}

			virtual void compress(const char *in) {
				compressor_->compressWith(enc_, in);
			}

			TEncoder& enc_;
			TRecordCompressor* compressor_;
		};

		template<
			typename TEncoder,
			typename TRecordCompressor
		>
		static dynamic_compressor::ptr make_dynamic_compressor(TEncoder& encoder, TRecordCompressor* compressor) {
			return dynamic_compressor::ptr(
					new dynamic_compressor1<TEncoder, TRecordCompressor>(encoder, compressor));
		}

		template<
			typename TDecoder,
			typename TRecordDecompressor
		>
		struct dynamic_decompressor1 : public dynamic_decompressor {
			dynamic_decompressor1(TDecoder& dec, TRecordDecompressor* decompressor) :
				dec_(dec), decompressor_(decompressor) {}

			virtual void decompress(char *in) {
				decompressor_->decompressWith(dec_, in);
			}

			~dynamic_decompressor1() {
				delete decompressor_;
			}

			TDecoder& dec_;
			TRecordDecompressor* decompressor_;
		};

		template<
			typename TDecoder,
			typename TRecordDecompressor
		>
		static dynamic_decompressor::ptr make_dynamic_decompressor(TDecoder& decoder, TRecordDecompressor* decompressor) {
			return dynamic_decompressor::ptr(
					new dynamic_decompressor1<TDecoder, TRecordDecompressor>(decoder, decompressor));
		}
	}
}

#endif // __formats_hpp__
