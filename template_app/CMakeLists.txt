cmake_minimum_required(VERSION 3.16)
project(HelloBruceApp C)

# ---- Configuration ----
# Set BRUCE_TARGET_ARCH to "esp32" or "esp32s3" depending on your device
set(BRUCE_TARGET_ARCH "esp32" CACHE STRING "Target architecture: esp32 or esp32s3")

# Toolchain — must match the target
if(BRUCE_TARGET_ARCH STREQUAL "esp32s3")
    set(CMAKE_C_COMPILER "xtensa-esp32s3-elf-gcc")
else()
    set(CMAKE_C_COMPILER "xtensa-esp32-elf-gcc")
endif()

# PIC and size-optimized flags
set(CMAKE_C_FLAGS "-fPIC -mlongcalls -Os -ffunction-sections -fdata-sections")

# Link as shared library with custom linker script
set(CMAKE_SHARED_LINKER_FLAGS "-nostdlib -T ${CMAKE_SOURCE_DIR}/app.ld -Wl,--gc-sections")

# ---- Build ----
add_library(hello_bruce SHARED hello_bruce.c)
target_include_directories(hello_bruce PRIVATE ../include)

# ---- Pack into .bruce ----
add_custom_command(TARGET hello_bruce POST_BUILD
    COMMAND python ${CMAKE_SOURCE_DIR}/../tools/pack.py
        $<TARGET_FILE:hello_bruce>
        ${CMAKE_SOURCE_DIR}/hello_bruce.bruce
        --arch ${BRUCE_TARGET_ARCH}
    COMMENT "Packing hello_bruce.bruce (${BRUCE_TARGET_ARCH})..."
)
