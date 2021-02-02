#!/bin/bash
echo "${LPMRC}" > ~/.npmrc
mkdir .publish
cp README.md package.json cskburn-win32.exe cskburn-linux .publish
cd .publish
npm --registry "${LPM_REGISTRY}" publish
