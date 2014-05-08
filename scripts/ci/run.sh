#!/bin/bash -e
# Builds and tests laz-perf
#

mkdir -p _build || exit 1
cd _build || exit 1

cmake \
	-DCMAKE_BUILD_TYPE=Release \
	..

make

./test/tlaz_tests
