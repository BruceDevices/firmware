#!/usr/bin/env python3
"""Validate and name the Heltec factory/Launcher images from one application build."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


TARGET = "heltec-v4-expansion-v2"
PREFIX = f"Bruce-{TARGET}"
TABLE_OFFSET = 0x8000
APP_OFFSET = 0x10000
FLASH_SIZE = 0x1000000
APP_CAPACITY = 0x470000
EXPECTED_PARTITIONS = (
    ("nvs", 1, 2, 0x9000, 0x6000),
    ("factory", 0, 0, APP_OFFSET, APP_CAPACITY),
    ("spiffs", 1, 0x82, 0x480000, 0xB70000),
    ("coredump", 1, 3, 0xFF0000, 0x10000),
)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def validate_image_header(data, description):
    if len(data) < 24 or data[0] != 0xE9 or not 1 <= data[1] <= 16:
        raise ValueError(f"{description}: invalid ESP image header")
    chip_id = struct.unpack_from("<H", data, 12)[0]
    if chip_id != 9:
        raise ValueError(f"{description}: expected ESP32-S3 chip ID 9, got {chip_id}")


def read_partitions(data):
    if not data or len(data) > 0x1000 or len(data) % 32:
        raise ValueError("Invalid partition-table length")
    partitions = []
    for offset in range(0, len(data), 32):
        record = data[offset : offset + 32]
        if record[:16] == b"\xeb\xeb" + b"\xff" * 14:
            if hashlib.md5(data[:offset]).digest() != record[16:]:
                raise ValueError("Partition-table MD5 mismatch")
            if data[offset + 32 :] != b"\xff" * (len(data) - offset - 32):
                raise ValueError("Unexpected data after partition-table MD5")
            break
        magic, kind, subtype, start, size, raw_label, flags = struct.unpack("<HBBII16sI", record)
        if magic != 0x50AA:
            raise ValueError("Missing partition-table MD5 or invalid entry")
        label = raw_label.split(b"\x00", 1)[0].decode("ascii")
        if flags or start < 0x9000 or size == 0 or start + size > FLASH_SIZE:
            raise ValueError(f"Invalid or unsupported partition: {label}")
        if any(start < entry[3] + entry[4] and start + size > entry[3] for entry in partitions):
            raise ValueError(f"Overlapping partition: {label}")
        partitions.append((label, kind, subtype, start, size))
    else:
        raise ValueError("Missing partition-table MD5")
    if tuple(partitions) != EXPECTED_PARTITIONS:
        raise ValueError("Partition layout differs from this target's custom_16Mb.csv")
    return partitions


def validate_build(merged, bootloader, partitions, application):
    validate_image_header(bootloader, "Bootloader")
    validate_image_header(application, "Application")
    layout = read_partitions(partitions)
    if len(bootloader) > TABLE_OFFSET:
        raise ValueError("Bootloader overlaps the partition table")
    if len(application) > APP_CAPACITY:
        raise ValueError("Application exceeds its factory partition")
    if len(merged) != APP_OFFSET + len(application):
        raise ValueError("Merged image must end at the application, without filesystem payload/padding")
    if merged[: len(bootloader)] != bootloader:
        raise ValueError("Merged bootloader differs from this build")
    if merged[TABLE_OFFSET : TABLE_OFFSET + len(partitions)] != partitions:
        raise ValueError("Merged partition table differs from this build")
    if merged[APP_OFFSET:] != application:
        raise ValueError("Merged application differs from this build")
    return layout


def package_release(project_dir, build_dir):
    project_dir = Path(project_dir)
    build_dir = Path(build_dir)
    original_name = f"{PREFIX}.bin"
    merged = (project_dir / original_name).read_bytes()
    bootloader = (build_dir / "bootloader.bin").read_bytes()
    partitions = (build_dir / "partitions.bin").read_bytes()
    application = (build_dir / "firmware.bin").read_bytes()
    layout = validate_build(merged, bootloader, partitions, application)

    names = [original_name, f"{PREFIX}-FACTORY-0x0.bin", f"{PREFIX}-LAUNCHER-INSTALL.bin"]
    for name in names[1:]:
        (project_dir / name).write_bytes(merged)
    digest = sha256(merged)
    checksums_name = f"{PREFIX}.sha256"
    (project_dir / checksums_name).write_text(
        "".join(f"{digest}  {name}\n" for name in names), encoding="utf-8"
    )

    metadata = {
        "board": TARGET,
        "chip": "ESP32-S3",
        "flash_bytes": FLASH_SIZE,
        "application": {
            "sha256": sha256(application),
            "size_bytes": len(application),
            "offset_in_package": APP_OFFSET,
            "maximum_size_bytes": APP_CAPACITY,
        },
        "partition_table_sha256": sha256(partitions),
        "partitions": [
            {"name": name, "type": kind, "subtype": subtype, "offset": offset, "size": size}
            for name, kind, subtype, offset, size in layout
        ],
        "artifacts": [
            {"name": name, "sha256": digest, "size_bytes": len(merged)} for name in names
        ],
        "factory_install": "Flash the complete FACTORY-0x0 image at address 0x0.",
        "launcher_install": (
            "Use Launcher Install on LAUNCHER-INSTALL.bin. This is a merged install image; "
            "Launcher reads its partition metadata and chooses destination partitions. "
            "It is not a raw application image."
        ),
        "launcher_format_review": {
            "source_commit": "2f43aa7bd3ef03e770c0a97135cbcfcc79c15aba",
            "source_repository": "https://github.com/bmorcelli/Launcher",
            "expected_default_empty_spiffs_bytes_on_16mb": 0x70000,
            "physical_installation_tested_by_packager": False,
        },
        "filesystem_payload_included": False,
        "hardware_validation": "Not established by packaging checks.",
    }
    (project_dir / f"{PREFIX}.json").write_text(
        json.dumps(metadata, indent=2) + "\n", encoding="utf-8"
    )
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", type=Path, default=Path.cwd())
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = package_release(args.project_dir, args.build_dir)
    except (OSError, ValueError) as error:
        raise SystemExit(f"Heltec packaging failed: {error}") from error
    print(f"Packaged {len(result['artifacts'])} verified images from one Heltec application build")


if __name__ == "__main__":
    main()
