#!/bin/bash

set -e
cd "$(dirname "$0")/.."
CMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

docker run -it --rm -v $(pwd):/src emscripten/emsdk:2.0.34 bash -c \
"
	rm -rf ./build && \
	mkdir -p ./build && \
	cd build && \
	cmake \
		-DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
		-DCMAKE_BUILD_TYPE=Release \
		.. && \
	emmake make VERBOSE=1
"
cp ./build/cpp/emscripten/laz-perf.js ./js/src/
cp ./build/cpp/emscripten/laz-perf.wasm ./js/src/
