@echo off
set GENERATOR="Visual Studio 15 Win64"

set BUILD_TYPE=Release

cmake -G %GENERATOR% ^
    -DCMAKE_INSTALL_PREFIX=C:\pdalbin ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_VERBOSE_MAKEFILE=OFF ^
	-DBUILD_SHARED_LIBS=ON ^
    -Dgtest_force_shared_crt=ON ^	
    .