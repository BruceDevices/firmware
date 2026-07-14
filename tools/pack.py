#!/usr/bin/env python3
"""
pack.py — Bruce App Package (BAP) packer
Prepends a BAP header to an ELF file to create a .bruce package.

Usage:
    python pack.py <input.elf> <output.bruce> [--arch esp32|esp32s3] [--name <app_name>]

BAP Header Layout (42 bytes, little-endian, packed):
    char    magic[4]    = "BAP1"
    uint8_t arch        = 0x01 (ESP32) | 0x02 (ESP32-S3)
    uint8_t version     = 1
    char    name[32]    = null-terminated app name
    uint32_t elf_size   = size of the ELF payload
"""

import sys
import struct
import os
import argparse

ARCH_MAP = {
    "esp32":   0x01,
    "esp32s3": 0x02,
}

def main():
    parser = argparse.ArgumentParser(description="Pack an ELF into a BAP (Bruce App Package)")
    parser.add_argument("input_elf", help="Path to the input ELF/shared library")
    parser.add_argument("output_bruce", help="Path for the output .bruce file")
    parser.add_argument("--arch", choices=ARCH_MAP.keys(), default="esp32",
                        help="Target architecture (default: esp32)")
    parser.add_argument("--name", default=None,
                        help="App name (default: derived from output filename)")
    args = parser.parse_args()

    if not os.path.exists(args.input_elf):
        print(f"Error: {args.input_elf} not found")
        sys.exit(1)

    with open(args.input_elf, 'rb') as f:
        elf_data = f.read()

    # Validate ELF magic
    if elf_data[:4] != b'\x7fELF':
        print(f"Error: {args.input_elf} is not a valid ELF file")
        sys.exit(1)

    elf_size = len(elf_data)
    bap_arch = ARCH_MAP[args.arch]

    # App name
    app_name = args.name or os.path.basename(args.output_bruce).replace('.bruce', '')
    name_bytes = app_name.encode('utf-8')[:31]
    name_padded = name_bytes + b'\x00' * (32 - len(name_bytes))

    # Pack header: magic(4) + arch(1) + version(1) + name(32) + elf_size(4) = 42 bytes
    header = struct.pack('<4sBB32sI', b'BAP1', bap_arch, 1, name_padded, elf_size)
    total_size = len(header) + elf_size

    with open(args.output_bruce, 'wb') as f:
        f.write(header)
        f.write(elf_data)

    print(f"[PACK] Package created: {args.output_bruce} ({total_size} bytes)")
    print(f"  Name: {app_name}")
    print(f"  Arch: {args.arch} (0x{bap_arch:02X})")
    print(f"  ELF size: {elf_size} bytes")
    print(f"  BAP size: {total_size} bytes")

if __name__ == "__main__":
    main()
