#!/bin/bash
mkdir -p out
pushd out > /dev/null
cmake .. && make -j4
popd > /dev/null
