#!/bin/bash
mkdir -p out
pushd out > /dev/null
cmake .. && cmake --build .
popd > /dev/null
