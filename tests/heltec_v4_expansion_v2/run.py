#!/usr/bin/env python3
"""Compile and run Heltec input and LoRa policy tests with a native C++ compiler."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def find_compiler(explicit):
    candidates = [explicit, os.environ.get("CXX"), shutil.which("g++"), shutil.which("clang++")]
    candidates.append(
        str(Path.home() / ".platformio" / "packages" / "toolchain-gccmingw32" / "bin" / "g++.exe")
    )
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise SystemExit("A native g++ or clang++ is required; pass its path with --compiler.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", help="Path to a native g++ or clang++ executable")
    args = parser.parse_args()
    compiler = find_compiler(args.compiler)
    test_directory = Path(__file__).resolve().parent
    repository = test_directory.parents[1]
    environment = os.environ.copy()
    environment["PATH"] = str(compiler.parent) + os.pathsep + environment.get("PATH", "")
    suites = [
        ("input_policy_tests", "input_policy_tests.cpp", []),
        ("lora_heltec_tests", "lora_band_tests.cpp", ["-DTEST_BOARD_LIMITS"]),
        ("lora_generic_tests", "lora_band_tests.cpp", []),
        ("file_hash_tests", "file_hash_tests.cpp", []),
    ]
    with tempfile.TemporaryDirectory(prefix="bruce-heltec-policy-") as temporary:
        for name, source, definitions in suites:
            executable = Path(temporary) / (name + (".exe" if os.name == "nt" else ""))
            subprocess.run(
                [
                    str(compiler), "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    *definitions, "-I", str(repository), str(test_directory / source),
                    "-o", str(executable),
                ],
                cwd=repository,
                env=environment,
                check=True,
            )
            subprocess.run([str(executable)], cwd=repository, env=environment, check=True)


if __name__ == "__main__":
    main()
