#!/bin/bash
mkdir -p out
pushd out > /dev/null
cmake .. -DWITHOUT_USB=YES && cmake --build .
popd > /dev/null
