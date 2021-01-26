#!/bin/bash
mkdir -p out
pushd out > /dev/null
make install
popd > /dev/null
