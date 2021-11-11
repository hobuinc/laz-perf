#!/bin/sh
if [ -z "$EMSDK" ] ; then
	echo "You need to set the EMSDK environment variable."
	exit 1
fi

cmake .. \
	-G "Unix Makefiles"\
    -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
	-DCMAKE_BUILD_TYPE=Release -DWASM=1
$EMSDK/upstream/emscripten/emmake make VERBOSE=1
