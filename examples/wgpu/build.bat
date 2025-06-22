@echo off

set configuration=debug
if "%1"=="release" (
    set configuration=release
)

set BUILD_TYPE=Debug
if "%configuration%"=="release" (
    set BUILD_TYPE=Release
)

set build_dir=build\%configuration%

if exist "%build_dir%" (
    rmdir /s /q %build_dir%
)
mkdir %build_dir%

cmake -B %build_dir% -G "MinGW Makefiles" -D CMAKE_BUILD_TYPE=%BUILD_TYPE%

copy /Y "roboto.ttf" %build_dir%
