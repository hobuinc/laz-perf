# What is this?
Exploration into compiling LASzip into faster javascript.

# Why?
The emscripten output for LASzip to javascript is unusably slow.  The generated Javascript code is not going to be a feasible solution to bring LASzip to all browsers.  This project will explore and benchmark certain aspects of LASzip decompression and then help decide on next steps.

# The plan

The goal is to get a minimum viable experiment to work which will help pick a path for further research and development.  For this purpose, decompresison of only point positions will be considered and implemented.

- Write a native javascript decoder and benchmark it.
- Templatize and reduce C++ runtime overhead in LASzip.  Templates can be used to tight-loop decompression.  This tight-looped code can (may be) then generate faster javascript code.  Benchmark the experimental C++ and the generated Javascript code.
