@echo off

set BUILD_DIR=build

pushd ..
if exist %BUILD_DIR% (
    echo Removing CMakeCache in %BUILD_DIR%
    del /q %BUILD_DIR%\CMakeCache.txt
    del /q %BUILD_DIR%\cmake_install.cmake
    rmdir /s /q %BUILD_DIR%\CMakeFiles
    rem keep intermediate and binaries untouched for now
) else (
    mkdir %BUILD_DIR%
    cd %BUILD_DIR%
)

echo Generating CMake solution in %BUILD_DIR%
pushd %BUILD_DIR%
cmake -DBUILD_TESTS=ON ..

popd
popd

echo Done!
pause