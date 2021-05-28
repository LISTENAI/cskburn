#!/bin/bash
docker build --platform linux/arm/v7 -f Dockerfile.alpine -t cskburn-arm-musl .
docker run --platform linux/arm/v7 --rm -v $PWD:/project cskburn-arm-musl ./test.sh
