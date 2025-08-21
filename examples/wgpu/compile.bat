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

if exist ripple.exe (
   del ripple.exe
)

make

if exist ripple.exe (
   gdb -batch -ex "set logging on" -ex run -ex "bt full" -ex quit --args ripple
)

cd ../..
endlocal
