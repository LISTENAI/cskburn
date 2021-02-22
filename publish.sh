#!/bin/bash
echo "${LPMRC}" > ~/.npmrc
cp README.md cskburn-win32.exe cskburn-linux cskburn-darwin npm/
cd npm
npm --registry "${LPM_REGISTRY}" publish
