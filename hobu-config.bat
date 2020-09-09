@echo on

:: This configure script is designed for the default Windows world, 

:: This configure script expects to be run from the PDAL root directory.

:: Pick your CMake GENERATOR.  (NMake will pick up architecture (x32, x64) from your environment)
set GENERATOR="NMake Makefiles"
REM set GENERATOR="Visual Studio 14 Win64"
rem set GENERATOR="Visual Studio 10"

:: Pick your build type
set BUILD_TYPE=Release
REM set BUILD_TYPE=Debug


if EXIST CMakeCache.txt del CMakeCache.txt

cmake -G %GENERATOR% ^
    -DCMAKE_INSTALL_PREFIX=c:/OSGeo4W64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_VERBOSE_MAKEFILE=OFF ^
	-DBUILD_SHARED_LIBS=ON ^
    .
	
