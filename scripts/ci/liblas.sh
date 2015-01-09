#!/bin/bash -e
# Installs LASzip library

git clone https://github.com/libLAS/liblas.git liblas --depth=1
cd liblas
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DWITH_LASZIP=ON
make
sudo make install

