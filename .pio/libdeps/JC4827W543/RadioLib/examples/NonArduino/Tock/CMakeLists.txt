# RadioLib Non-Arduino Tock Library CMake script
#
# Licensed under the MIT License
#
# Copyright (c) 2023 Alistair Francis <alistair@alistair23.me>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.18)

# create the project
project(tock-sx1261)

set(LINKER_SCRIPT $ENV{LIBTOCK_C_DIRECTORY}/userland_generic.ld)

if (RISCV_BUILD)
    include("tock-riscv.cmake")
else()
    include("tock-arm.cmake")
endif()

# when using debuggers such as gdb, the following line can be used
#set(CMAKE_BUILD_TYPE Debug)

# if you did not build RadioLib as shared library (see wiki),
# you will have to add it as source directory
# the following is just an example, yours will likely be different
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

# add the executable
add_executable(${PROJECT_NAME} main.cpp)

# link with RadioLib and libtock-c
# The build system for libtock-c is a bit odd and the version of libraries
# built changes based on compiler version.
if (RISCV_BUILD)
    if(EXISTS "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0")
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/rv32imc/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/rv32imc/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0/riscv/lib/gcc/riscv64-unknown-elf/14.1.0/rv32i/ilp32/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.4.0.20231231/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.4.0.20231231/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libm.a
        )

        target_include_directories(RadioLib AFTER PUBLIC
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/riscv/riscv64-unknown-elf/include/
        )
    elseif(EXISTS "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0")
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/rv32imc/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/rv32imc/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0/riscv/lib/gcc/riscv64-unknown-elf/13.2.0/rv32i/ilp32/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libm.a
        )

        target_include_directories(RadioLib AFTER PUBLIC
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/riscv/riscv64-unknown-elf/include/
        )
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/rv32imc/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/rv32imc/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-10.5.0/riscv/lib/gcc/riscv64-unknown-elf/10.5.0/rv32i/ilp32/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-10.5.0/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.2.0.20211231/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.2.0.20211231/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libm.a
        )

        target_include_directories(RadioLib AFTER PUBLIC
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.2.0.20211231/riscv/riscv64-unknown-elf/include/
        )
    endif()
else()
    if (EXISTS "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0")
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/cortex-m4/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/cortex-m4/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0/arm/lib/gcc/arm-none-eabi/14.1.0/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-14.1.0/arm/arm-none-eabi/lib/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.4.0.20231231/arm/arm-none-eabi/lib/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.4.0.20231231/arm/arm-none-eabi/lib/libm.a
        )
    elseif(EXISTS "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0")
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/cortex-m4/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/cortex-m4/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0/arm/lib/gcc/arm-none-eabi/13.2.0/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-13.2.0/arm/arm-none-eabi/lib/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/arm/arm-none-eabi/lib/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.3.0.20230120/arm/arm-none-eabi/lib/libm.a
        )
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC
            RadioLib
            $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/cortex-m4/libtock.a
            $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/cortex-m4/libtocksync.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-10.5.0/arm/lib/gcc/arm-none-eabi/10.5.0/libgcc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-10.5.0/arm/arm-none-eabi/lib/libstdc++.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.2.0.20211231/arm/arm-none-eabi/lib/libc.a
            $ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-4.2.0.20211231/arm/arm-none-eabi/lib/libm.a
        )
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/
    $ENV{LIBTOCK_C_DIRECTORY}
)

# you can also specify RadioLib compile-time flags here
#target_compile_definitions(${PROJECT_NAME} PUBLIC RADIOLIB_DEBUG RADIOLIB_VERBOSE)
