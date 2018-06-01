#!/bin/sh
# Download and place sets required to run test suites
#

if [ ! -e test/raw-sets/autzen.laz ] ; then
	cd test/raw-sets && \
		curl -O https://github.com/PDAL/data/raw/master/autzen/autzen.laz && \
		las2las autzen.laz autzen.las && \
		cd ../..
fi
