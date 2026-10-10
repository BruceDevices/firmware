#!/usr/bin/env python3
"""Compile production EMV parser and synthetic transcripts with ASan/UBSan."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    exe = Path(directory) / 'emv-test'
    subprocess.run(['g++', '-std=c++17', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie',
                    '-I', str(root / 'src/modules/rfid'), str(root / 'tests/test_emv_protocol.cpp'),
                    str(root / 'src/modules/rfid/emv_protocol.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
