from pathlib import Path
import subprocess
import sys
import importlib.util


def _check_format():
    folders = [
        "src",
        "boards",
        "include",
    ]

    extensions = {".c", ".cpp", ".cc", ".h", ".hpp", ".hh"}

    for folder in folders:
        p = Path(folder)

        if not p.exists():
            continue

        for file in p.rglob("*"):
            if file.suffix.lower() in extensions:
                subprocess.run(
                    ["clang-format", "-i", str(file)],
                    check=True
                )


def _check_clang_format(target, source, env):
    _check_format()
    env.Exit(0)


def _setup_platformio():
    from SCons.Script import Import

    Import("env")

    env.AddCustomTarget(
        name="clang-format",
        dependencies=None,
        actions=[_check_clang_format],
        title="Apply Clang format",
        description="Apply Clang format"
    )


# Detects if running inside PlatformIO/SCons
try:
    from SCons.Script import Import
    running_in_platformio = True
except ModuleNotFoundError:
    running_in_platformio = False

# runs the checker before every build
_check_format()

# Optional manual target for platformio: `pio run -t clang-format`
# Run clang-format without building the project
if running_in_platformio:
    _setup_platformio()
