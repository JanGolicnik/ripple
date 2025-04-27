if not exist build (
   build.bat
)
cd build

if exist ripple.exe (
   del ripple.exe
)

make

if exist ripple.exe (
   gdb -batch -ex "set logging on" -ex run -ex "bt full" -ex quit --args ripple
)

cd ..
