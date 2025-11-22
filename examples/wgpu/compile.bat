@echo off
setlocal

set configuration=debug
if "%1"=="release" (
    set configuration=release
)

set build_dir=build\%configuration%

if not exist %build_dir% (
   build.bat %configuration%
)

copy /Y "shader.wgsl" %build_dir%\

cd %build_dir%

if exist ripple_test.exe (
   del ripple_test.exe
)

make

if "%2"=="" (
if exist ripple_test.exe (
   gdb -batch -ex "set logging on" -ex run -ex "bt full" -ex quit --args ripple_test
)
)

cd ../..
endlocal & exit /b %BUILD_RESULT%
