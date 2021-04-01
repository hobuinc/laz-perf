
# What is this?

Alternative [LAZ](http://laszip.org) implementation. It supports compilation to WASM via
[Emscripten](https://emscripten.org/) so that LAZ data can be decoded in a browser.  This
project provides an alternative implementation to the [LAStools](http://lastools.org/) library
that provides a more rigorous software engineering approach.

# Building LAZperf for Windows/UNIX?

Previous version of LAZperf were header-only C++ libraries, so you could simply include the
project header files in your project. Primarily due to licensing issues, this is no longer the
case and LAZperf needs to be built as a library that you link with your code. LAZperf uses
CMake as a build system, though it's probably simple to port to another build system as there
are few source files. Assuming you have Git, CMake, make and C++11 compiler installed, here is the
process on the Unix command line. The process is similar on Windows.

    git clone https://github.com/hobu/laz-perf.git
    cd laz-perf
    mkdir build
    cd build
    cmake ..
    make

This should build the library `liblazperf.so` (or similar). You can install this library along
with the supporting header files as follows:

    make install

# Using LAZperf on Windows/UNIX

Although the LAZperf library is focused on decoding the LAZ data itself, there is support
for reading a complete LAS or LAZ file. If you have LAZ-comrpessed data, you can decompress
by creating a decompressor for the right point type and providing a callback that will
provide data from the LAZ source as requested by the decompressor. For example, to read
point format 0 data, you might do the following:

    using namespace lazperf;

    void cb(unsigned char *buf, int len)
    {
        static unsigned char my_laz_data[] = {...};
        static int idx = 0;

        std::copy(buf, buf + len, my_laz_data + idx);
        idx += len;
    }

    point_decompressor_0 decompressor(cb);

    char pointbuf[100];
    for (int i = 0; i < num_points; ++i)
        decompressor(pointbuf);

Compression follows a similar pattern -- see the accompanying examples and tests.

You can also use LAZperf to read LAZ data from an entire LAZ or LAS file:

    using namespace lazperf;

    reader::named_file f(filename);

    char pointbuf[100];
    for (size_t i = 0; i < f.header().point_count; ++i)
        c.readPoint(pointbuf);

A memory file interface exists If your LAS/LAZ data is internal rather than in a file:

    using namespace lazperf;

    reader::mem_file f(buf, bufsize);

    char pointbuf[100];
    for (size_t i = 0; i < f.header().point_count; ++i)
        c.readPoint(pointbuf);


# Buiding LAZperf for use in a browser.

In order to build LAZperf for the browser, you must have the
[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed.
You will also need Git and CMake.
Activate the installation to set the necessary EMSDK environment variable then follow
these steps:

    git clone https://github.com/hobu/laz-perf.git
    cd laz-perf
    mkdir build
    cd build
    . ../emscripten-build.sh

This should create two files in the subdirectory build/cpp/emscripten: laz-perf.js and
laz-perf.wasm. Both are necessary for running LAZperf from the browser.

# Using LAZperf in a browser

See the file cpp/emscripten/index.html for an example of how to decode LAZ data in
the browser. Note that laz-perf.js will fetch laz-perf.wasm when run. You don't need
to fetch it manually.

