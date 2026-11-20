#!/usr/bin/env python3
"""Host tests for the board-local factory/Launcher release packager."""

import hashlib
import json
from pathlib import Path
import runpy
import struct
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = runpy.run_path(str(ROOT / "boards/heltec-v4-expansion-v2/package_release.py"))
PREFIX = "Bruce-heltec-v4-expansion-v2"


def image(size=256, chip_id=9):
    value = bytearray(b"\x00" * size)
    value[0] = 0xE9
    value[1] = 1
    struct.pack_into("<H", value, 12, chip_id)
    return bytes(value)


def partition_table(filesystem_size=0xB70000):
    records = (
        ("nvs", 1, 2, 0x9000, 0x6000),
        ("factory", 0, 0, 0x10000, 0x470000),
        ("spiffs", 1, 0x82, 0x480000, filesystem_size),
        ("coredump", 1, 3, 0xFF0000, 0x10000),
    )
    data = b"".join(
        struct.pack("<HBBII16sI", 0x50AA, kind, subtype, offset, size, name.encode("ascii"), 0)
        for name, kind, subtype, offset, size in records
    )
    data += b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(data).digest()
    return data.ljust(0xC00, b"\xff")


def merge(boot, table, app):
    data = bytearray(b"\xff" * (0x10000 + len(app)))
    data[: len(boot)] = boot
    data[0x8000 : 0x8000 + len(table)] = table
    data[0x10000:] = app
    return bytes(data)


class PackagingTests(unittest.TestCase):
    def setUp(self):
        self.boot = image(512)
        self.table = partition_table()
        self.app = image(4096)
        self.merged = merge(self.boot, self.table, self.app)

    def validate(self, merged=None, boot=None, table=None, app=None):
        return PACKAGE["validate_build"](
            self.merged if merged is None else merged,
            self.boot if boot is None else boot,
            self.table if table is None else table,
            self.app if app is None else app,
        )

    def test_valid_images_preserve_standalone_partition_layout(self):
        layout = self.validate()
        self.assertEqual(layout[1], ("factory", 0, 0, 0x10000, 0x470000))
        self.assertEqual(layout[2], ("spiffs", 1, 0x82, 0x480000, 0xB70000))

    def test_wrong_board_application_is_rejected(self):
        for chip in (0, 2, 5, 12):
            with self.subTest(chip=chip):
                app = image(4096, chip_id=chip)
                with self.assertRaisesRegex(ValueError, "ESP32-S3"):
                    self.validate(app=app, merged=merge(self.boot, self.table, app))

    def test_wrong_board_bootloader_is_rejected(self):
        boot = image(512, chip_id=0)
        with self.assertRaisesRegex(ValueError, "Bootloader.*ESP32-S3"):
            self.validate(boot=boot, merged=merge(boot, self.table, self.app))

    def test_invalid_image_headers_are_rejected(self):
        for app in (b"", b"\xe9", b"\x00" * 64, b"\xe9\x00" + b"\x00" * 62):
            with self.subTest(size=len(app)):
                with self.assertRaisesRegex(ValueError, "image header"):
                    self.validate(app=app)

    def test_partition_md5_is_checked(self):
        table = bytearray(self.table)
        table[12] ^= 1
        with self.assertRaisesRegex(ValueError, "MD5"):
            self.validate(table=bytes(table))

    def test_unknown_partition_layout_is_rejected_even_with_valid_md5(self):
        table = partition_table(filesystem_size=0x100000)
        with self.assertRaisesRegex(ValueError, "Partition layout"):
            self.validate(table=table, merged=merge(self.boot, table, self.app))

    def test_missing_partition_md5_is_rejected(self):
        table = self.table[:128] + b"\xff" * (len(self.table) - 128)
        with self.assertRaisesRegex(ValueError, "MD5"):
            self.validate(table=table)

    def test_data_after_partition_md5_is_rejected(self):
        table = self.table[:-1] + b"\x00"
        with self.assertRaisesRegex(ValueError, "after partition-table MD5"):
            self.validate(table=table)

    def test_oversized_application_is_rejected(self):
        app = image(0x470001)
        with self.assertRaisesRegex(ValueError, "exceeds"):
            self.validate(app=app, merged=merge(self.boot, self.table, app))

    def test_application_exactly_at_limit_is_accepted(self):
        app = image(0x470000)
        self.validate(app=app, merged=merge(self.boot, self.table, app))

    def test_stale_or_modified_merge_is_rejected(self):
        for offset, message in ((32, "bootloader"), (0x8020, "partition table"), (0x10020, "application")):
            with self.subTest(offset=offset):
                merged = bytearray(self.merged)
                merged[offset] ^= 1
                with self.assertRaisesRegex(ValueError, message):
                    self.validate(merged=bytes(merged))

    def test_truncation_padding_and_filesystem_payload_are_rejected(self):
        for merged in (self.merged[:-1], self.merged + b"\xff", self.merged + b"\x00" * 1024):
            with self.subTest(size=len(merged)):
                with self.assertRaisesRegex(ValueError, "must end at the application"):
                    self.validate(merged=merged)

    def test_packages_and_checksums_use_identical_application_build(self):
        with tempfile.TemporaryDirectory(prefix="bruce-heltec-package-test-") as temporary:
            project = Path(temporary)
            build = project / "build"
            build.mkdir()
            (build / "bootloader.bin").write_bytes(self.boot)
            (build / "partitions.bin").write_bytes(self.table)
            (build / "firmware.bin").write_bytes(self.app)
            (project / f"{PREFIX}.bin").write_bytes(self.merged)
            metadata = PACKAGE["package_release"](project, build)
            names = (
                f"{PREFIX}.bin",
                f"{PREFIX}-FACTORY-0x0.bin",
                f"{PREFIX}-LAUNCHER-INSTALL.bin",
            )
            self.assertEqual([entry["name"] for entry in metadata["artifacts"]], list(names))
            self.assertEqual(metadata["application"]["sha256"], hashlib.sha256(self.app).hexdigest())
            for name in names:
                data = (project / name).read_bytes()
                self.assertEqual(data, self.merged)
                self.assertEqual(data[0x10000:], self.app)
            sums = (project / f"{PREFIX}.sha256").read_text().splitlines()
            self.assertEqual(len(sums), 3)
            for line, name in zip(sums, names):
                self.assertEqual(line, f"{hashlib.sha256(self.merged).hexdigest()}  {name}")
            self.assertEqual(json.loads((project / f"{PREFIX}.json").read_text()), metadata)
            self.assertFalse(metadata["filesystem_payload_included"])

    def test_invalid_build_does_not_create_install_artifacts(self):
        with tempfile.TemporaryDirectory(prefix="bruce-heltec-package-test-") as temporary:
            project = Path(temporary)
            build = project / "build"
            build.mkdir()
            (build / "bootloader.bin").write_bytes(self.boot)
            (build / "partitions.bin").write_bytes(self.table)
            (build / "firmware.bin").write_bytes(self.app)
            (project / f"{PREFIX}.bin").write_bytes(self.merged[:-1])
            with self.assertRaises(ValueError):
                PACKAGE["package_release"](project, build)
            self.assertFalse((project / f"{PREFIX}-LAUNCHER-INSTALL.bin").exists())
            self.assertFalse((project / f"{PREFIX}-FACTORY-0x0.bin").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
