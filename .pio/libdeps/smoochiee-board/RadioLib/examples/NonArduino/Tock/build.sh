#!/bin/bash
set -e

rm -rf ./build-*

pushd ${LIBTOCK_C_DIRECTORY}/examples/cxx_hello
make -j4
popd

mkdir -p build-arm
cd build-arm

cmake -G "CodeBlocks - Unix Makefiles" ..
make -j4

cd ..

if ! env | grep SKIP_RISCV; then
	mkdir -p build-riscv
	cd build-riscv

	cmake -G "CodeBlocks - Unix Makefiles" -DRISCV_BUILD=1 ..
	make -j4

	cd ..
fi

elf2tab -n radio-lib --stack 4096 --app-heap 2048 --kernel-heap 2048 \
	--kernel-major 2 --kernel-minor 1 \
	-v ./build-arm/tock-sx1261
