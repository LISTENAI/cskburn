#!/bin/bash
cmake -B build
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
