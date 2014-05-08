#!/bin/bash -e
# Installs requirements for las-perf
# Borrowed from PDAL's pre-script.

sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 16126D3A3E5C1192
sudo apt-get update -y
sudo apt-get install software-properties-common -y
sudo apt-get install python-software-properties -y
sudo add-apt-repository ppa:boost-latest/ppa -y
sudo apt-get update -qq
sudo apt-get install \
	cmake \
	boost1.55

cd $TRAVIS_BUILD_DIR
