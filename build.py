from pathlib import Path
from struct import Struct
from SCons.Script import Import

# Import PlatformIO's SCons environment
Import("env")
senv = env  # avoid shadowing with the action's 'env' parameter

# Optional: keep your extra flag
senv.Append(CXXFLAGS=["-Wno-conversion-null"])

# ---- Detect MCU and map bootloader offsets ----
board_config = senv.BoardConfig()
mcu = (board_config.get("build.mcu") or senv.get("BOARD_MCU") or "").lower()

BOOT_OFFSETS = {
    "esp32":   0x1000,  # ESP32
    "esp32s3": 0x0000,  # ESP32-S3
    "esp32c5": 0x2000,  # ESP32-C5
    "esp32c6": 0x0000,  # ESP32-C6
    "esp32p4": 0x2000,  # ESP32-P4
}
boot_offset = BOOT_OFFSETS.get(mcu, 0x0000)  # safe fallback

# Default offsets
PART_TABLE_OFFSET = 0x8000

# Paths
build_dir = Path(senv.subst("$BUILD_DIR"))
proj_dir  = Path(senv.subst("$PROJECT_DIR"))
pioenv    = senv.subst("${PIOENV}")

boot_bin = build_dir / "bootloader.bin"
part_bin = build_dir / "partitions.bin"
app_bin  = build_dir / "firmware.bin"

out_bin  = proj_dir / f"Bruce-{pioenv}.bin"

python_exe  = senv.get("PYTHONEXE", "python")

chip_arg = mcu if mcu else "esp32"


def _check_app_size(env):
    # Read the table that will be flashed, including resolved offsets and sizes.
    entry = Struct("<HBBII16sI")
    try:
        app_offset = int(env.subst("$ESP32_APP_OFFSET") or "0x10000", 0)
        partitions = part_bin.read_bytes()
        if len(partitions) % entry.size:
            raise ValueError("partition table has an incomplete entry")
        fw_size = app_bin.stat().st_size
        for magic, kind, _, offset, size, _, _ in entry.iter_unpack(partitions):
            if magic != 0x50AA:
                break
            if kind == 0 and offset == app_offset and size:
                print(f"[app_size] App uses {fw_size:,} of {size:,} bytes at {app_offset:#x}")
                if not 0 < fw_size <= size:
                    raise ValueError("firmware.bin is empty or exceeds its app partition")
                return app_offset
        raise ValueError(f"no app partition at {app_offset:#x}")
    except (OSError, ValueError) as error:
        print(f"[app_size] Error: {error}")
        env.Exit(1)


def _check_upload_app(target, source, env):
    _check_app_size(env)


def _merge_bins_callback(target, source, env):
    """
    Post-action callback executed after firmware.bin is built.
    Merges bootloader, partitions, and app into a single binary.
    NOTE: This function signature must be (target, source, env) so SCons can call it.
    """
    # Check files
    missing = [p for p in [boot_bin, part_bin, app_bin] if not p.exists()]
    if missing:
        print("[merge_bin] Missing files, merge aborted:")
        for p in missing:
            print(f" - {p}")
        env.Exit(1)

    # Quote paths for Windows safety
    def q(p): return f"\"{p}\""

    app_offset = _check_app_size(env)

    cmd = " ".join([
        q(python_exe), "-m", "platformio", "pkg", "exec", "-p", q("tool-esptoolpy"), "--", "esptool.py",
        "--chip", chip_arg,
        "merge-bin",
        "--output", q(out_bin),
        hex(boot_offset), q(boot_bin),
        hex(PART_TABLE_OFFSET), q(part_bin),
        hex(app_offset), q(app_bin),
    ])

    print("[merge_bin] Merging binaries:")
    print(" ", cmd)
    rc = env.Execute(cmd)
    if rc != 0:
        print(f"[merge_bin] Failed with exit code {rc}")
        env.Exit(rc)
    print(f"[merge_bin] Success -> {out_bin} ({out_bin.stat().st_size} bytes)")

# Automatically run after firmware.bin is generated
senv.AddPostAction(str(app_bin), _merge_bins_callback)
senv.AddPreAction("upload", _check_upload_app)

# Optional manual target: depend on the three binaries and run the same callback
# Wrap it in a lambda so SCons can call it without (target, source, env) if needed.
senv.AddCustomTarget(
    name="build-firmware",
    dependencies=[str(boot_bin), str(part_bin), str(app_bin)],
    actions=[_merge_bins_callback],
    title="Build Firmware (merge)",
    description="Merge bootloader + partitions + app into a single .bin file"
)

# Keep your upload-nobuild helper
senv.AddCustomTarget(
    name="upload-nobuild",
    dependencies=None,
    actions=[f"platformio run -t upload -t nobuild -e {pioenv}"],
    title="Upload Nobuild",
    description="Run upload without rebuilding"
)
