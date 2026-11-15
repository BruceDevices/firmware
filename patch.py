import hashlib
import os
import subprocess
from typing import TYPE_CHECKING, Any
import requests

if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}

import glob
import gzip
from os import makedirs, remove, rename
from os.path import basename, dirname, exists, isfile, join

Import("env")  # type: ignore

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32-libs")
board_mcu = env.BoardConfig()
mcu = board_mcu.get("build.mcu", "")
patchflag_path = join(FRAMEWORK_DIR,mcu, "lib", ".patched")

# Resolve the installed toolchain directly; current pioarduino uses unified
# xtensa-esp-elf binaries rather than the former per-MCU package names.
if mcu != "esp32p4" and not isfile(patchflag_path):
    original_file = join(FRAMEWORK_DIR, mcu, "lib", "libnet80211.a")
    patched_file = original_file + ".patched"
    if mcu in ("esp32c5", "esp32c6"):
        candidates = [("toolchain-riscv32-esp", "riscv32-esp-elf-objcopy"),
                      ("toolchain-riscv32-esp-elf", "riscv32-esp-elf-objcopy")]
    else:
        candidates = [("toolchain-xtensa-esp-elf", "xtensa-esp-elf-objcopy"),
                      ("toolchain-xtensa-" + mcu, "xtensa-" + mcu + "-elf-objcopy")]
    objcopy = None
    for package, binary in candidates:
        try:
            package_dir = env.PioPlatform().get_package_dir(package)
        except (KeyError, ValueError):
            continue
        if not package_dir:
            continue
        candidate = join(package_dir, "bin", binary + (".exe" if os.name == "nt" else ""))
        if isfile(candidate):
            objcopy = candidate
            break
    if not objcopy:
        raise RuntimeError("Cannot find the installed ESP32 objcopy tool")
    subprocess.run([objcopy, "--weaken-symbol=ieee80211_raw_frame_sanity_check",
                    original_file, patched_file], check=True)
    # Do not move the original archive until objcopy has succeeded.
    if not isfile(patched_file):
        raise RuntimeError("objcopy did not produce the patched WiFi archive")
    if isfile(original_file + ".old"):
        remove(original_file + ".old")
    rename(original_file, original_file + ".old")
    rename(patched_file, original_file)
    with open(patchflag_path, "w"):
        pass


def hash_file(file_path):
    """Generate SHA-256 hash for a single file."""
    hasher = hashlib.sha256()
    with open(file_path, "rb") as f:
        # Read the file in chunks to avoid memory issues
        for chunk in iter(lambda: f.read(4096), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def hash_files(file_paths):
    """Generate a combined hash for multiple files."""
    combined_hash = hashlib.sha256()

    for file_path in file_paths:
        file_hash = hash_file(file_path)
        combined_hash.update(file_hash.encode("utf-8"))  # Update with the file's hash

    return combined_hash.hexdigest()


def save_checksum_file(hash_value, output_file):
    """Save the hash value to a specified output file."""
    with open(output_file, "w") as f:
        f.write(hash_value)


def load_checksum_file(input_file):
    """Load the hash value from a specified input file."""
    with open(input_file, "r") as f:
        return f.readline().strip()


def minify_resource(source, kind):
    raw = source.read()
    if os.environ.get("BRUCE_OFFLINE_WEB_ASSETS") == "1":
        return raw
    try:
        response = requests.post(
            "https://www.toptal.com/developers/" + kind + "-minifier/api/raw"
            if kind != "css" else "https://www.toptal.com/developers/cssminifier/api/raw",
            {"input": raw.decode("utf-8")}, timeout=(5, 20))
        response.raise_for_status()
        return response.content if response.content.strip() else raw
    except requests.RequestException:
        # Original assets remain functional; gzip still runs locally.
        print("[WEB] Minifier unavailable; embedding original " + kind + " asset")
        return raw


def minify_css(c):
    return minify_resource(c, "css")


def minify_js(js):
    return minify_resource(js, "javascript")


def minify_html(html):
    return minify_resource(html, "html")


# gzip web files
def prepare_www_files():
    HEADER_FILE = join(env.get("PROJECT_DIR"), "include", "webFiles.h")
    filetypes_to_gzip = ["html", "css", "js"]
    data_src_dir = join(env.get("PROJECT_DIR"), "embedded_resources/web_interface")
    checksum_file = join(data_src_dir, "checksum.sha256")
    checksum = ""

    if not exists(data_src_dir):
        print(f'Error: Source directory "{data_src_dir}" does not exist!')
        return

    if exists(checksum_file):
        checksum = load_checksum_file(checksum_file)

    files_to_gzip = []
    for extension in filetypes_to_gzip:
        files_to_gzip.extend(glob.glob(join(data_src_dir, "*." + extension)))

    files_checksum = hash_files(files_to_gzip)
    if files_checksum == checksum:
        print("[GZIP & EMBED INTO HEADER] - Nothing to process.")
        return

    print(f"[GZIP & EMBED INTO HEADER] - Processing {len(files_to_gzip)} files.")

    makedirs(dirname(HEADER_FILE), exist_ok=True)

    with open(HEADER_FILE, "w") as header:
        header.write(
            "#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n#include <Arduino.h>\n\n"
        )
        header.write(
            "// THIS FILE IS AUTOGENERATED DO NOT MODIFY IT. MODIFY FILES IN /embedded_resources/web_interface\n\n"
        )

        for file in files_to_gzip:
            gz_file = file + ".gz"
            with open(file, "rb") as src, gzip.open(gz_file, "wb") as dst:
                ext = basename(file).rsplit(".", 1)[-1].lower()
                if ext == 'html':
                    minified = minify_html(src)
                elif ext == 'css':
                    minified = minify_css(src)
                elif ext == 'js':
                    minified = minify_js(src)
                else:
                    raise ValueError(f"Unsupported file type: {ext}")

                # # Output minified file
                # min_file = file + ".min"
                # with open(min_file, "wb") as minf:
                #     minf.write(minified)

                dst.write(minified)

            with open(gz_file, "rb") as gz:
                compressed_data = gz.read()
                var_name = basename(file).replace(".", "_")

                header.write(f"const uint8_t {var_name}[] PROGMEM = {{\n")

                # Write hex values, inserting a newline every 15 bytes
                for i in range(0, len(compressed_data), 15):
                    hex_chunk = ", ".join(
                        f"0x{byte:02X}" for byte in compressed_data[i : i + 15]
                    )
                    header.write(f"  {hex_chunk},\n")

                header.write("};\n\n")
                header.write(
                    f"const uint32_t {var_name}_size = {len(compressed_data)};\n\n"
                )

            remove(gz_file)  # Clean up temporary gzip file

        header.write("#endif // WEB_FILES_H\n")

    save_checksum_file(files_checksum, checksum_file)

    print(f"[DONE] Gzipped files embedded into {HEADER_FILE}")


prepare_www_files()
