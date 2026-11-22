import runpy
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType
from unittest.mock import Mock, patch


ROOT = Path(__file__).resolve().parents[1]


def partition(kind, subtype, offset, size):
    return struct.pack('<HBBII16sI', 0x50AA, kind, subtype, offset, size, b'test', 0)


class FirmwareMergeTest(unittest.TestCase):
    def setUp(self):
        self.temp = self.enterContext(tempfile.TemporaryDirectory())
        self.root = Path(self.temp)
        self.app = self.root / 'firmware.bin'
        self.table = self.root / 'partitions.bin'
        (self.root / 'bootloader.bin').touch()
        self.env = Mock()
        self.env.BoardConfig.return_value = {'build.mcu': 'esp32s3', 'build.partitions': 'custom_16Mb.csv'}
        self.env.subst.side_effect = {
            '$BUILD_DIR': self.temp, '$PROJECT_DIR': self.temp, '${PIOENV}': 'test', '$ESP32_APP_OFFSET': '0x10000',
        }.get
        self.env.get.side_effect = lambda _, default=None: default
        self.env.PioPlatform.return_value.get_package_dir.return_value = self.temp
        self.env.Execute.side_effect = self.write_merged_file
        self.env.Exit.side_effect = SystemExit
        scons = ModuleType('SCons.Script')
        scons.Import = Mock()
        with patch.dict(sys.modules, {'SCons.Script': scons}):
            runpy.run_path(str(ROOT / 'build.py'), init_globals={'env': self.env})
        self.merge = next(call.args[1] for call in self.env.AddPostAction.call_args_list
                          if call.args[0] == str(self.app))

    def write_merged_file(self, _):
        (self.root / 'Bruce-test.bin').touch()
        return 0

    def set_image_size(self, size):
        with self.app.open('wb') as image:
            image.truncate(size)

    def factory_table(self):
        self.table.write_bytes(partition(0, 0, 0x10000, 0x470000)
                               + partition(1, 0x82, 0x480000, 0xB70000) + b'\xff' * 32)

    def test_factory_image_at_limit_can_be_merged(self):
        self.factory_table()
        self.set_image_size(0x470000)
        self.merge(None, None, self.env)
        self.env.Execute.assert_called_once()
        self.assertIn(f'0x10000 "{self.app}"', self.env.Execute.call_args.args[0])

    def test_factory_image_one_byte_over_limit_is_rejected_before_merge(self):
        self.factory_table()
        self.set_image_size(0x470001)
        with self.assertRaises(SystemExit):
            self.merge(None, None, self.env)
        self.env.Execute.assert_not_called()

    def test_nondefault_app_offset_selects_the_correct_partition(self):
        self.table.write_bytes(partition(0, 0, 0x10000, 0x10000) + partition(0, 0x10, 0x20000, 0x20000))
        self.env.subst.side_effect = lambda _: '0x20000'
        self.set_image_size(0x20000)
        self.merge(None, None, self.env)
        self.assertIn(f'0x20000 "{self.app}"', self.env.Execute.call_args.args[0])

    def test_invalid_partition_tables_fail_closed(self):
        cases = [b'truncated', b'\xff' * 32, partition(1, 0x82, 0x10000, 0x470000)]
        self.set_image_size(1)
        for table in cases:
            with self.subTest(table=table):
                self.table.write_bytes(table)
                with self.assertRaises(SystemExit):
                    self.merge(None, None, self.env)
        self.env.Execute.assert_not_called()

    def test_upload_checks_an_existing_image_before_flashing(self):
        upload = next(call.args[1] for call in self.env.AddPreAction.call_args_list if call.args[0] == 'upload')
        self.factory_table()
        self.set_image_size(0x470000)
        upload(None, None, self.env)
        self.set_image_size(0x470001)
        with self.assertRaises(SystemExit):
            upload(None, None, self.env)
        self.table.unlink()
        with self.assertRaises(SystemExit):
            upload(None, None, self.env)
