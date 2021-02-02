#!/bin/bash
echo "${LPMRC}" > ~/.npmrc
mkdir .publish
cp README.md package.json cskburn-win32.exe cskburn-linux cskburn-darwin .publish
cd .publish
npm --registry "${LPM_REGISTRY}" publish
