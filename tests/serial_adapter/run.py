"""Exercise the actual USBSerial/SerialDevice headers with a recording Stream."""

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
    args = parser.parse_args()
    compiler = shutil.which(args.cxx)
    if compiler is None:
        parser.error("C++ compiler not found; pass --cxx with its executable path")
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
    fixture = repo / "tests/serial_adapter"
    with tempfile.TemporaryDirectory(prefix="bruce-serial-adapter-") as temporary:
        binary = Path(temporary) / ("serial-adapter.exe" if os.name == "nt" else "serial-adapter")
        subprocess.run(
            [compiler, "-std=c++14", "-I" + str(fixture / "stubs"),
             "-I" + str(repo / "src"), "-I" + str(repo / "include"),
             str(fixture / "test_serial_adapter.cpp"), "-o", str(binary)],
            check=True,
            env=env,
        )
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
