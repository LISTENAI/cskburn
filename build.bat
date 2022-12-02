@ECHO OFF
cmake -B build -G Ninja
cmake --build build --config Release
