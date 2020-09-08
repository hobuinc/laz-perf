#!/bin/sh
if [ -z "$EMSCRIPTEN" ] ; then
	echo "You need to set the EMSCRIPTEN environment variable."
	exit 1
fi

cd cpp
cmake . -DCMAKE_TOOLCHAIN_FILE="$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" \
	-DCMAKE_BUILD_TYPE=Release
$EMSCRIPTEN/emmake make VERBOSE=1
