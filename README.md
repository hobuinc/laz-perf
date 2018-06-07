[![Build Status](https://travis-ci.org/hobu/laz-perf.svg?branch=master)](https://travis-ci.org/hobu/laz-perf)
[![AppVeyor Status](https://ci.appveyor.com/api/projects/status/o3qv0njdw5hvk47f)](https://ci.appveyor.com/project/hobu/laz-perf/)

# What is this?
Alternative [LAZ](http://laszip.org) implementation. It supports compilation and usage
in JavaScript, usage in database contexts such as pgpointcloud and Oracle Point Cloud, and
it executes faster than the LASzip codebase.

# Why?
The Emscripten/WebAssembly output for LASzip to JavaScript was unusably slow.  The generated
JavaScript code is not going to be a feasible solution to bring LASzip to all
browsers.  This project provides an alternative implementation that plays
nice with Emscripten and provides a more rigorous software engineering approach
to a LASzip implementation.

# How do I build this?
You need to download the most recent version of Emscripten toolchain from [Emscripten's Web Page](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html) and follow their setup process.

Once done, navigate to the root directory of laz-perf project and make a directory to stage build files in:

    git clone https://github.com/hobu/laz-perf.git 
    cd laz-perf
    mkdir build ; cd build

Then run `cmake` like so:

    cmake .. \
        -DEMSCRIPTEN=1 \
        -DCMAKE_TOOLCHAIN_FILE=<path-to-emsdk>/emscripten/<emsdk-version>/cmake/Modules/Platform/Emscripten.cmake

To perform a WebAssembly build, pass the `-DWASM=1` parameter to the command above.

You should now be able to build JS/WASM output like so:

    VERBOSE=1 make


    



# Benchmark results so far

All tests were run on a 2013 Macbook Pro `2.6 Ghz Intel Core i7 16GB 1600 MHz DD3`.  Arithmetic encoder was run on a 4 field struct with two signed and two unsigned fields.  Please see the `benchmarks/brute.cpp` for how these tests were run.  The emscriten version used was `Emscripten v1.14.0, fastcomp LLVM, JS host: Node v0.10.18`

### Native:

          Count       Comp Init       Comp Time      Comp Flush     Decomp Init     Decomp Time
           1000        0.000001        0.000279        0.000000        0.000000        0.000297
          10000        0.000000        0.001173        0.000000        0.000000        0.001512
         100000        0.000000        0.009104        0.000000        0.000000        0.011168
        1000000        0.000000        0.082419        0.000000        0.000000        0.108797

### Node.js, test runtime JS v0.10.25

          Count       Comp Init       Comp Time      Comp Flush     Decomp Init     Decomp Time
           1000        0.000586        0.014682        0.000273        0.000383        0.008012
          10000        0.000022        0.017960        0.000009        0.000004        0.020219
         100000        0.000030        0.128615        0.000008        0.000004        0.141459
        1000000        0.000010        1.245053        0.000009        0.000005        1.396419

### Firefox, v28.0
          Count       Comp Init       Comp Time      Comp Flush     Decomp Init     Decomp Time
           1000        0.000005        0.001311        0.000006        0.000003        0.000820
          10000        0.000003        0.007966        0.000004        0.000001        0.007299
         100000        0.000001        0.062016        0.000003        0.000001        0.064037
        1000000        0.000002        0.662454        0.000009        0.000003        0.673866

### Google Chrome, v34.0.1847.116

          Count       Comp Init       Comp Time      Comp Flush     Decomp Init     Decomp Time
           1000        0.000751        0.012357        0.000424        0.000516        0.008413
          10000        0.000016        0.006971        0.000016        0.000004        0.009481
         100000        0.000008        0.059768        0.000009        0.000004        0.070253
        1000000        0.000009        0.576017        0.000019        0.000005        0.658435
