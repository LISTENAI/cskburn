#!/bin/bash
mkdir -p out
pushd out > /dev/null
cmake --install .
popd > /dev/null
