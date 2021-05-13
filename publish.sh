#!/bin/bash
echo "${NPMRC}" > ~/.npmrc
cp README.md npm/
cp cskburn-win32-x64.exe npm/
cp cskburn-linux-x64 npm/
cp cskburn-linux-arm npm/
cp package_template.json npm/package.json
cd npm
sed -i "s|{{NAME}}|${PKG_NAME}|" package.json
sed -i "s|{{VERSION}}|${PKG_VERSION}|" package.json
npm publish --access public
