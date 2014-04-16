// common functions
// 

#ifndef __common_hpp__
#define __common_hpp__

#include <chrono>

namespace common {
	std::chrono::time_point<std::chrono::high_resolution_clock> tick() {
		return std::chrono::high_resolution_clock::now();
	}

	float since(const std::chrono::time_point<std::chrono::high_resolution_clock>& p) {
		using namespace std::chrono;

		auto now = high_resolution_clock::now();
		return duration_cast<duration<float> >(now - p).count();
	}
}


#endif // __common_hpp__
