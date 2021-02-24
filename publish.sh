#!/bin/bash
echo "${NPMRC}" > ~/.npmrc
cp README.md cskburn-win32.exe cskburn-linux cskburn-darwin npm/
cp package_template.json npm/package.json
cd npm
sed -i "s|{{NAME}}|${PKG_NAME}|" package.json
sed -i "s|{{VERSION}}|${PKG_VERSION}|" package.json
npm publish --access public
