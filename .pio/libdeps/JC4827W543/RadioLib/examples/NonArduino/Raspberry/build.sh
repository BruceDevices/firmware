#!/bin/bash

set -e
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make
cd ..
size build/rpi-sx1261
