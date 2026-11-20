cd build_gcc
 
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER:FILEPATH=gcc.exe -DCMAKE_CXX_COMPILER:FILEPATH=g++.exe ../
 
copy compile_commands.json ..\

cd ..

