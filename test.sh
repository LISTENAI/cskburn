#!/bin/bash
mkdir -p out
pushd out > /dev/null
cmake .. && cmake --build . && ctest --output-on-failure
popd > /dev/null
