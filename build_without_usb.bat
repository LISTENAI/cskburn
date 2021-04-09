@ECHO OFF
IF NOT EXIST out mkdir out
pushd out
cmake .. -DWITHOUT_USB=YES && cmake --build . --config Release
popd
