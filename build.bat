@ECHO OFF
IF NOT EXIST out mkdir out
pushd out
cmake .. && cmake --build .
popd