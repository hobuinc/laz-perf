#!/bin/bash

# This script generates:
# The actual WASM:
#   src/laz-perf.wasm
#
# The bootstrapping JS built for all environments:
#   src/laz-perf.js
#
# And the bootstrapping JS built for specific environments:
#   lib/node/laz-perf.js
#   lib/web/laz-perf.js
#   lib/worker/laz-perf.js

set -e
cd "$(dirname "$0")/.."
CMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

mkdir -p ./js/build
function build () {
	ENVIRONMENT="$1"
	printf "\n\nBuilding for environment: ${ENVIRONMENT}\n"
	docker run --rm -v $(pwd):/src emscripten/emsdk:3.1.20 bash -c \
	"
		cd js/build && \
		cmake \
			-DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
			-DCMAKE_BUILD_TYPE=Release \
			-DENVIRONMENT=${ENVIRONMENT} \
			../.. && \
		emmake make VERBOSE=1
	"

	# This one is a generic build intended for use in all environments.
	# Note that the generated WASM will be the same for all of these builds,
	# only the JS bootstrapping code will vary based on the environment.
	if [ "$ENVIRONMENT" = "all" ]; then
		cp ./js/build/cpp/emscripten/laz-perf.js ./js/src/
		cp ./js/build/cpp/emscripten/laz-perf.wasm ./js/src/
    # And these are the individual bootstrappers for the various environments.
	else
        mkdir -p ./js/lib/${ENVIRONMENT}
		cp ./js/build/cpp/emscripten/laz-perf.js ./js/lib/${ENVIRONMENT}/
	fi
}

build all
build node
build web
build worker
