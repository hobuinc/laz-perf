// factory.hpp
// Factory to create dynamic compressors and decompressors
//

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
				return make_dynamic_decompressor(enc,
						new formats::record_decompressor<field<las::point10> >());
			}

			std::cout << "Schema is " << hash << std::endl;


			// we got a schema we don't know how to build
			throw unknown_schema_type();
			return dynamic_decompressor::ptr(); // avoid warning
		}
	}
};

#endif // __factory_hpp__
