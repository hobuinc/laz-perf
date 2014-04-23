# What is this?
Exploration into compiling LASzip into faster javascript.

# Why?
The emscripten output for LASzip to javascript is unusably slow.  The generated Javascript code is not going to be a feasible solution to bring LASzip to all browsers.  This project will explore and benchmark certain aspects of LASzip decompression and then help decide on next steps.

# The plan

The goal is to get a minimum viable experiment to work which will help pick a path for further research and development.  For this purpose, decompresison of only point positions will be considered and implemented.

- Write a native javascript decoder and benchmark it.
- Templatize and reduce C++ runtime overhead in LASzip.  Templates can be used to tight-loop decompression.  This tight-looped code can (may be) then generate faster javascript code.  Benchmark the experimental C++ and the generated Javascript code.

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