#!/usr/bin/env bash

BUILD_DIR=../../src/spmv/build

find ${BUILD_DIR} -maxdepth 2 -name "_build.log" -printf "\n%p\n" -exec tail {} \;
