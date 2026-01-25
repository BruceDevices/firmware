Import("env")
import os
import subprocess
import hashlib
import shutil

PIOENV = env.subst("$PIOENV")
MQJS_PATH = os.path.join(".pio/libdeps", PIOENV, "mquickjs")

BUILD_DIR = "lib/mquickjs_headers"
GEN = os.path.join(BUILD_DIR, "mqjs_stdlib_generator")
BUILD_SHA256 = os.path.join(BUILD_DIR, "mqjs_stdlib.build.sha256")

BJS_INTERPRETER_PATH = "src/modules/bjs_interpreter/"
WATCH_FILE = os.path.join(BJS_INTERPRETER_PATH, "mqjs_stdlib.c")

SRC = [
    WATCH_FILE,
    os.path.join(MQJS_PATH, "mquickjs_build.c"),
]

CFLAGS = [
    "-Wall",
    "-O2",
    "-m32",
    "-I" + MQJS_PATH,
]

HOST_CC = "gcc"

INCLUDES = [
    'user_classes_js',
    'buffer_js',
    'audio_js',
    'badusb_js',
    'device_js',
    'display_js',
    'dialog_js',
    'globals_js',
    'gpio_js',
    'i2c_js',
    'ir_js',
    'keyboard_js',
    'math_js',
    'mic_js',
    'notification_js',
    'rfid_js',
    'runtime_js',
    'serial_js',
    'storage_js',
    'subghz_js',
    'wifi_js',
]

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(8192)
            if not chunk:
                break
            # Normalize CRLF to LF so signatures are stable across OSes.
            h.update(chunk.replace(b"\r\n", b"\n"))
    return h.hexdigest()

def _project_option(name: str):
    try:
        return env.GetProjectOption(name, default="")
    except Exception:
        return ""

def _resolve_host_cc():
    override = os.environ.get("MQJS_HOST_CC") or _project_option("mqjs_host_cc")
    return override if override else HOST_CC

def _has_define(name: str) -> bool:
    try:
        flags = env.ParseFlags(env['BUILD_FLAGS'])
    except Exception:
        return False
    for define in flags.get('CPPDEFINES', []):
        if define == name:
            return True
        if isinstance(define, list) and define and define[0] == name:
            return True
    return False

def _gen_enabled() -> bool:
    # Default to enabled for backward compatibility.
    if _has_define("DISABLE_MQJS_HEADERS_GEN"):
        return False
    if _has_define("GEN_MQJS_HEADERS"):
        value = get_build_flag_value("GEN_MQJS_HEADERS")
        if value is not None and str(value).strip().lower() in ("0", "false", "no", "off"):
            return False
    return True

def _normalize_option_value(value) -> str:
    if value is None:
        return ""
    if isinstance(value, (list, tuple)):
        # Keep order; PlatformIO preserves flag order.
        return "\n".join(_normalize_option_value(v) for v in value)
    return str(value)

def compute_signature():
    # Any change here should force a rebuild.
    parts = []
    parts.append("v=3")
    parts.append(f"watch_sha256={sha256_file(WATCH_FILE)}")

    mqjs_build_c = os.path.join(MQJS_PATH, "mquickjs_build.c")
    parts.append(f"mquickjs_build_sha256={sha256_file(mqjs_build_c)}")
    mqjs_build_h = os.path.join(MQJS_PATH, "mquickjs_build.h")
    parts.append(f"mquickjs_build_sha256={sha256_file(mqjs_build_h)}")

    h = hashlib.sha256()
    h.update("\n".join(parts).encode("utf-8"))
    return h.hexdigest()

def needs_rebuild():
    if not os.path.exists(os.path.join(BJS_INTERPRETER_PATH, "mqjs_stdlib.h")):
        return True
    if not os.path.exists(BUILD_SHA256):
        return True

    with open(BUILD_SHA256, "r") as f:
        old = f.read().strip()

    print(old, compute_signature())
    return compute_signature() != old

def write_build_stamp():
    with open(BUILD_SHA256, "w") as f:
        f.write(compute_signature())

def get_build_flag_value(flag_name):
    build_flags = env.ParseFlags(env['BUILD_FLAGS'])
    flags_with_value_list = [build_flag for build_flag in build_flags.get('CPPDEFINES') if type(build_flag) == list]
    defines = {k: v for (k, v) in flags_with_value_list}
    return defines.get(flag_name)

def generate_headers():
    if not os.path.exists(MQJS_PATH):
        return

    if get_build_flag_value("LITE_VERSION") is not None or get_build_flag_value("DISABLE_INTERPRETER") is not None:
        return

    if not _gen_enabled():
        print("Skipped MQJS headers build.")
        return

    os.makedirs(BUILD_DIR, exist_ok=True)

    if not needs_rebuild():
        return

    try:
        host_cc = _resolve_host_cc()
        if not shutil.which(host_cc):
            if os.path.exists(os.path.join(BJS_INTERPRETER_PATH, "mqjs_stdlib.h")):
                print("Host compiler not found; skipping mqjs_stdlib header generation.")
                print("Set MQJS_HOST_CC or mqjs_host_cc to a GCC-compatible compiler, or use Docker.")
                print("\nOn Windows:\n- install MSYS2 https://www.msys2.org/ at default path")
                print("- open C:\\msys64\\ucrt64.exe")
                print("- run on UCRT64 terminal: pacman -S --needed mingw-w64-i686-toolchain")
                print("- add C:\\msys64\\mingw32\\bin to PATH, cmd: setx PATH \"%PATH%;C:\\msys64\\mingw32\\bin\\\"")
                print("- close and open VSCode again")
                print("\nOn Mac:\n- run on terminal: xcode-select --install")
                print("\nOn Linux:\n- run on terminal: sudo apt-get install -y --no-install-recommends gcc-multilib libc6-dev-i386\n\n")
                return
            raise RuntimeError("Host compiler not found and mqjs_stdlib.h is missing.")

        # Build the generator as a native host executable. The output headers are
        # still forced to 32-bit target format via the generator's `-m32` flag.
        gcc_result = subprocess.run(
            [host_cc, *CFLAGS, "-o", GEN, *SRC],
            capture_output=True,
            text=True,
        )
        if gcc_result.returncode != 0:
            if gcc_result.stdout:
                print("gcc stdout:\n" + gcc_result.stdout)
            if gcc_result.stderr:
                print("gcc stderr:\n" + gcc_result.stderr)
            raise subprocess.CalledProcessError(
                gcc_result.returncode,
                gcc_result.args,
                output=gcc_result.stdout,
                stderr=gcc_result.stderr,
            )

        print("gen_mqjs_headers.py Generating QuickJS headers for 32-bit targets")

        with open(os.path.join(BJS_INTERPRETER_PATH, "mqjs_stdlib.h"), "w") as f:
            result = subprocess.run([GEN, "-m32"], capture_output=True, text=True, check=True)
            for line in INCLUDES:
                f.write(f'#include "{line}.h"\n')
            f.write("\n")
            f.write(result.stdout)

        with open(os.path.join(BUILD_DIR, "mquickjs_atom.h"), "w") as f:
            subprocess.check_call([GEN, "-a", "-m32"], stdout=f)

    except Exception as e:
        print("\nError generating MicroQuickJS headers (gen_mqjs_headers.py).")
        print("This error occurs because the mqjs_stdlib.c file was modified.")
        print("If you want to make changes to this file, you need to install build-essential tools and ensure that gcc is available.")
        print("Alternatively, you can build using Docker by running:")
        print("  docker compose run platformio_build\n")
        raise e

    write_build_stamp()

generate_headers()
