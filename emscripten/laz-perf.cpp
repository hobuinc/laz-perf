// laz-perf.cpp
// javascript bindings for laz-perf
//

#include <emscripten/bind.h>
#include <iostream>

using namespace emscripten;

class LASZip {
	public:
		LASZip() {}
		void open(unsigned int b, size_t len) {
			const char *buf = (const char*) b;
			std::cout << "Size is: " << len << std::endl;
		}
};

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);

	class_<LASZip>("LASZip")
		.constructor()
		.function("open", &LASZip::open, allow_raw_pointers());
}
