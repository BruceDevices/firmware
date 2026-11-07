"""Compile and exercise the production pin configuration with host I/O stubs."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    repo = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument(
        "--arduinojson",
        type=Path,
        default=repo / ".pio/libdeps/heltec-v4-expansion-v2/ArduinoJson/src",
        help="Directory containing the project's installed ArduinoJson.h",
    )
    args = parser.parse_args()
    compiler = shutil.which(args.cxx)
    if compiler is None:
        parser.error("C++ compiler not found; pass --cxx with its executable path")
    if not (args.arduinojson / "ArduinoJson.h").is_file():
        parser.error("ArduinoJson dependency missing; install the board's PlatformIO dependencies first")
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
    fixture = repo / "tests/heltec_pins"
    common = [
        compiler,
        "-std=c++14",
        "-DARDUINOJSON_ENABLE_ARDUINO_STRING=0",
        "-include", str(fixture / "stubs/host_fs.h"),
        "-I" + str(fixture / "stubs"),
        "-I" + str(repo / "boards/heltec-v4-expansion-v2"),
        "-I" + str(repo / "src/core"),
        "-I" + str(args.arduinojson.resolve()),
        str(repo / "src/core/configPins.cpp"),
        str(fixture / "test_config_pins.cpp"),
    ]
    with tempfile.TemporaryDirectory(prefix="bruce-heltec-pins-") as temporary:
        for name, flags in (
            ("heltec", ["-DHELTEC_V4_EXPANSION_V2"]),
            ("other-board", []),
        ):
            binary = Path(temporary) / (name + (".exe" if os.name == "nt" else ""))
            subprocess.run(common + flags + ["-o", str(binary)], check=True, env=env)
            subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
