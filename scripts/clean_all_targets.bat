@echo off

set BUILD_DIR=build

pushd ..
if not exist %BUILD_DIR% (
    echo No %BUILD_DIR% folder in the root one!! Execute regenerate script.
    pause
    exit 1
)

echo Building CMake solution in %BUILD_DIR%
pushd %BUILD_DIR%

cmake --build . -t=clean --config=Debug
cmake --build . -t=clean --config=Release

popd
popd

echo Done!
pause
