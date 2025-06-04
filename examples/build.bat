@echo off
if exist build (
    rmdir /s /q build
)
mkdir build
cmake -B build -G "MinGW Makefiles" -D CMAKE_BUILD_TYPE=Debug

copy /Y "..\include\raylib\lib\raylib.dll" "build\"
