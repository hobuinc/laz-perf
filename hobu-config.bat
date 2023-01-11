@echo on

set GENERATOR="Ninja"
set BUILD_TYPE=Release

del /s /q %CONDA_DEFAULT_ENV%
mkdir %CONDA_DEFAULT_ENV%
cd %CONDA_DEFAULT_ENV%


cmake -G %GENERATOR% ^
    -DCMAKE_INSTALL_PREFIX=%CONDA_PREFIX% ^
     -DCMAKE_BUILD_TYPE=Release -Dgtest_force_shared_crt=ON ^
    -DCMAKE_VERBOSE_MAKEFILE=OFF ^
    ..
    

ninja