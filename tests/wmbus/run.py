#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Build and run the production decoder and simulated CC1101 receiver tests."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++', help='C++ compiler executable')
    parser.add_argument('--zig', action='store_true', help='Use python -m ziglang c++')
    parser.add_argument('--build-dir', type=Path, help='Override the build directory')
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    root = tests.parents[1]
    source = root / 'src/modules/rf'
    build = (args.build_dir or root / '.pio/wmbus-host-tests').resolve()
    build.mkdir(parents=True, exist_ok=True)
    compiler = [sys.executable, '-m', 'ziglang', 'c++'] if args.zig else [args.cxx]
    flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG', '-fsanitize=undefined',
             '-fno-sanitize-recover=undefined', '-Wall', '-Wextra', '-Werror',
             '-I' + str(source), '-I' + str(tests)]

    # Compile the exact production Receiver implementation with a fake CC1101.
    # The UI and SD code after remember() are excluded from this host executable.
    production = (source / 'rf_wmbus.cpp').read_text(encoding='utf-8')
    begin = production.index('namespace {')
    end = production.index('\nvoid remember(')
    receiver_source = build / 'test_receiver.cpp'
    receiver_source.write_text(
        '#include "fake_cc1101.h"\n' + production[begin:end] + '\n}\n' +
        (tests / 'test_receiver_main.cpp').read_text(encoding='utf-8'),
        encoding='utf-8')
    suffix = '.exe' if sys.platform == 'win32' else ''
    for name, test_source in [('test_wmbus', tests / 'test_wmbus.cpp'),
                              ('test_receiver', receiver_source)]:
        executable = build / (name + suffix)
        subprocess.run(compiler + flags + [str(source / 'wmbus_codec.cpp'),
                       str(test_source), '-o', str(executable)], check=True)
        result = subprocess.run([str(executable)], check=True, capture_output=True, text=True)
        print(result.stdout, end='', flush=True)
        if result.stderr:
            print(result.stderr, file=sys.stderr, end='')
        (build / (name + '-result.txt')).write_text(result.stdout, encoding='utf-8')


if __name__ == '__main__':
    main()
