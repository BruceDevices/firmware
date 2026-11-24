"""Run focused host regressions without PlatformIO or a connected board."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
compiler = shutil.which("g++")
if not compiler:
    raise SystemExit("BLOCKED: g++ is required for the host regression")
with tempfile.TemporaryDirectory(prefix="bruce-encoder-input-") as temporary:
    binary = Path(temporary) / "encoder-input.exe"
    subprocess.run(
        [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(root / "include"),
         str(root / "tools/test_encoder_input.cpp"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
