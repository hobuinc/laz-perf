// formats.hpp
// Format support
//

#ifndef __formats_hpp__
#define __formats_hpp__

#include <iostream>
#include "compressor.hpp"

namespace laszip {
	namespace formats {
		template<typename T>
		struct field {
			static_assert(std::is_arithmetic<T>::value,
					"Fields can only be arithmetic types at this time");

			typedef T type;
		};

		template<typename TEncoder, typename... TS>
		struct record;

		template<typename TEncoder>
		struct record<TEncoder> {
			record(TEncoder&) {}

			inline void encode(const char *buffer) {
				printf("Ending! %p\n", buffer);
			}

			inline void decode(char *buffer) {
				printf("Ending! %p\n", buffer);
			}
		};

		template<typename TEncoder, typename T, typename... TS>
		struct record<TEncoder, T, TS...> {
			record(TEncoder& enc) : compressor_(enc, sizeof(typename T::type) * 8), next_(enc) { }

			inline void encode(const char *buffer) {
				typedef typename T::type this_type;

				std::cout << "Reading " << sizeof(this_type) << " bytes and posting forward..."
					<< std::endl;
				next_.encode(buffer + sizeof(this_type));
			}

			inline void decode(char *buffer) {
				typedef typename T::type this_type;

				std::cout << "Writing " << sizeof(this_type) << " bytes and posting forward..."
					<< std::endl;
				next_.decode(buffer + sizeof(this_type));
			}

			laszip::compressors::integer<TEncoder> compressor_;
			record<TEncoder, TS...> next_;
		};
	}
}

#endif // __formats_hpp__
