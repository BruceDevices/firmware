from __future__ import annotations

from pathlib import Path
import re

try:
    Import("env")
except Exception:  # Running outside PlatformIO/SCons
    env = None


NIMCONFIG_BLOCK = """\
#ifdef CONFIG_IDF_TARGET_ESP32P4
# undef NIMBLE_CFG_CONTROLLER
# define NIMBLE_CFG_CONTROLLER 0
# undef CONFIG_BT_CONTROLLER_ENABLED
# ifndef CONFIG_NIMBLE_CPP_IDF
#  define CONFIG_NIMBLE_CPP_IDF (1)
# endif
#endif
"""


INCLUDE_REGEX = re.compile(r'^\s*#\s*include\s+[<"]esp_bt\.h[>"]')
IF_DIRECTIVE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b')
ENDIF_DIRECTIVE = re.compile(r'^\s*#\s*endif\b')
TARGET_IFNDEF = re.compile(r'^\s*#\s*ifndef\s+CONFIG_IDF_TARGET_ESP32P4\b')
P4_FILE_GUARD_BEGIN = "#if 0 /* TAB5_NIMBLE_PATCH */\n"
P4_FILE_GUARD_END = "#endif /* TAB5_NIMBLE_PATCH */\n"
OLD_P4_FILE_GUARD_BEGIN = "#if !defined(CONFIG_IDF_TARGET_ESP32P4) /* TAB5_NIMBLE_PATCH */\n"

PATCH_MARKER = "/* TAB5_NIMBLE_PATCH */"

# NimBLEDevice::init() still guards the legacy esp_bt controller bring-up (which needs
# esp_bt.h symbols absent on the P4) behind `... || defined(USING_NIMBLE_ARDUINO_HEADERS)`.
# That macro is always defined for the Arduino build, so on the P4 the block is compiled and
# fails (esp_bt_controller_config_t / esp_bt_controller_enable undeclared). Append
# `&& !defined(CONFIG_IDF_TARGET_ESP32P4)` to that single #if so the P4 skips it. Match the
# controlling directive by its content, independent of upstream whitespace/version bumps.
CONTROLLER_INIT_IF = re.compile(
    r'^(?P<prefix>\s*#\s*if\s+)(?P<expr>.*ESP_IDF_VERSION\s*<\s*ESP_IDF_VERSION_VAL\(\s*5\s*,\s*0\s*,\s*0\s*\)'
    r'.*defined\s*\(\s*USING_NIMBLE_ARDUINO_HEADERS\s*\).*?)\s*$'
)


def _repo_root() -> Path:
    if env is not None:
        return Path(env.subst("$PROJECT_DIR")).resolve()
    if "__file__" in globals():
        return Path(__file__).resolve().parents[2]
    return Path.cwd().resolve()


def _ensure_nimconfig(nimconfig_path: Path) -> None:
    if not nimconfig_path.exists():
        return

    text = nimconfig_path.read_text(encoding="utf-8", errors="ignore")
    if "CONFIG_IDF_TARGET_ESP32P4" in text and "NIMBLE_CFG_CONTROLLER 0" in text:
        return

    insert_at = text.rfind("\n#endif")
    if insert_at == -1:
        insert_at = text.rfind("#endif")
    if insert_at == -1:
        new_text = text.rstrip() + "\n\n" + NIMCONFIG_BLOCK + "\n"
    else:
        new_text = (
            text[:insert_at].rstrip()
            + "\n\n"
            + NIMCONFIG_BLOCK
            + "\n"
            + text[insert_at:].lstrip()
        )

    if new_text != text:
        nimconfig_path.write_text(new_text, encoding="utf-8")


def _wrap_esp_bt_includes(lib_root: Path) -> None:
    if not lib_root.exists():
        return

    for path in lib_root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in {".h", ".hpp", ".c", ".cpp", ".cc", ".ino"}:
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        if "esp_bt.h" not in text:
            continue

        lines = text.splitlines(keepends=True)
        stack = []
        changed = False
        out_lines = []

        i = 0
        while i < len(lines):
            line = lines[i]
            stripped = line.strip()

            if TARGET_IFNDEF.match(line):
                stack.append(True)
            elif IF_DIRECTIVE.match(line):
                stack.append(False)
            elif ENDIF_DIRECTIVE.match(line):
                if stack:
                    stack.pop()

            if INCLUDE_REGEX.match(line):
                if any(stack):
                    out_lines.append(line)
                else:
                    out_lines.append("#ifndef CONFIG_IDF_TARGET_ESP32P4\n")
                    out_lines.append(line if line.endswith("\n") else line + "\n")
                    out_lines.append("#endif\n")
                    changed = True
            else:
                out_lines.append(line)

            i += 1

        if changed:
            new_text = "".join(out_lines)
            if new_text != text:
                path.write_text(new_text, encoding="utf-8")


def _guard_file_for_esp32p4(path: Path) -> None:
    if not path.exists():
        return

    text = path.read_text(encoding="utf-8", errors="ignore")
    if text.startswith(OLD_P4_FILE_GUARD_BEGIN):
        text = text.replace(OLD_P4_FILE_GUARD_BEGIN, P4_FILE_GUARD_BEGIN, 1)
        path.write_text(text, encoding="utf-8")
        return

    if text.startswith(P4_FILE_GUARD_BEGIN):
        return

    if "TAB5_NIMBLE_PATCH" in text:
        return

    new_text = P4_FILE_GUARD_BEGIN + text
    if not new_text.endswith("\n"):
        new_text += "\n"
    new_text += P4_FILE_GUARD_END
    path.write_text(new_text, encoding="utf-8")


def _disable_conflicting_esp32p4_sources(lib_root: Path) -> None:
    files = (
        lib_root / "src" / "nimble" / "nimble" / "transport" / "esp_ipc" / "src" / "hci_esp_ipc.c",
        lib_root / "src" / "nimble" / "porting" / "npl" / "freertos" / "src" / "npl_os_freertos.c",
    )
    for file_path in files:
        _guard_file_for_esp32p4(file_path)


def _patch_controller_init(path: Path) -> None:
    if not path.exists():
        return

    text = path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines(keepends=True)
    changed = False

    for i, line in enumerate(lines):
        if PATCH_MARKER in line:
            continue
        match = CONTROLLER_INIT_IF.match(line)
        if not match:
            continue
        newline = "\n" if line.endswith("\n") else ""
        expr = match.group("expr").rstrip()
        lines[i] = (
            f"{match.group('prefix')}({expr}) && !defined(CONFIG_IDF_TARGET_ESP32P4) "
            f"{PATCH_MARKER}{newline}"
        )
        changed = True

    if changed:
        path.write_text("".join(lines), encoding="utf-8")


MSYS_PATCH_MARKER = "TAB5_NIMBLE_MSYS_PATCH"

# File-scope C-linkage declarations (extern "C" is illegal at block scope) for the two
# transport helpers we call. Both are defined in the bundled transport.c as C symbols.
MSYS_DECL_ANCHOR = "bool NimBLEDevice::init(const std::string& deviceName) {\n"
MSYS_DECL_BLOCK = (
    "#ifdef CONFIG_IDF_TARGET_ESP32P4 /* TAB5_NIMBLE_MSYS_PATCH */\n"
    "extern \"C\" esp_err_t ble_buf_alloc(void);\n"
    "extern \"C\" void ble_buf_free(void);\n"
    "extern \"C\" void ble_transport_init(void);\n"
    "extern \"C\" void ble_transport_deinit(void);\n"
    "#endif\n"
    "bool NimBLEDevice::init(const std::string& deviceName) {\n"
)

MSYS_INIT_ANCHOR = "        nimble_port_init();\n"
MSYS_INIT_BLOCK = (
    "#ifdef CONFIG_IDF_TARGET_ESP32P4 /* TAB5_NIMBLE_MSYS_PATCH */\n"
    "        // On the P4 the BLE controller lives on the ESP-Hosted co-processor, so the legacy\n"
    "        // esp_nimble_hci_init() path is disabled. That path is the only caller of\n"
    "        // ble_buf_alloc() (allocates the msys + transport mbuf buffers; without it\n"
    "        // os_msys_init() asserts on NULL) and ble_transport_init() (runs os_mempool_init on\n"
    "        // the HCI cmd/evt pools; without it the first advertising command faults in\n"
    "        // os_memblock_get on an uninitialised pool). esp_nimble_init() runs neither, so do\n"
    "        // both here, in the same order esp_nimble_hci_init() would.\n"
    "        ble_buf_alloc();\n"
    "        ble_transport_init();\n"
    "#endif\n"
    "        nimble_port_init();\n"
)

MSYS_DEINIT_ANCHOR = "            nimble_port_deinit();\n"
MSYS_DEINIT_BLOCK = (
    "            nimble_port_deinit();\n"
    "#ifdef CONFIG_IDF_TARGET_ESP32P4 /* TAB5_NIMBLE_MSYS_PATCH */\n"
    "            // Mirror esp_nimble_hci_deinit(): clear the transport pools and free the buffers\n"
    "            // so repeated BLE start/stop (Bruce cycles BLE frequently) neither leaks nor\n"
    "            // reuses freed pool memory on the next init.\n"
    "            ble_transport_deinit();\n"
    "            ble_buf_free();\n"
    "#endif\n"
)


def _patch_msys_buf_alloc(path: Path) -> None:
    """P4 controller-disabled builds never reach esp_nimble_hci_init(), which is the only
    caller of ble_buf_alloc()/os_msys_buf_alloc(). Without it os_msys_init() asserts on NULL
    mbuf buffers. Allocate them right before nimble_port_init() and free them after
    nimble_port_deinit(). Idempotent via the MSYS_PATCH_MARKER."""
    if not path.exists():
        return

    text = path.read_text(encoding="utf-8", errors="ignore")
    if MSYS_PATCH_MARKER in text:
        return

    changed = False
    if MSYS_DECL_ANCHOR in text:
        text = text.replace(MSYS_DECL_ANCHOR, MSYS_DECL_BLOCK, 1)
        changed = True
    if MSYS_INIT_ANCHOR in text:
        text = text.replace(MSYS_INIT_ANCHOR, MSYS_INIT_BLOCK, 1)
        changed = True
    if MSYS_DEINIT_ANCHOR in text:
        text = text.replace(MSYS_DEINIT_ANCHOR, MSYS_DEINIT_BLOCK, 1)
        changed = True

    if changed:
        path.write_text(text, encoding="utf-8")


def main() -> None:
    root = _repo_root()
    lib_root = root / ".pio" / "libdeps" / "m5stack-tab5" / "NimBLE-Arduino"
    _ensure_nimconfig(lib_root / "src" / "nimconfig.h")
    _wrap_esp_bt_includes(lib_root)
    _disable_conflicting_esp32p4_sources(lib_root)
    _patch_controller_init(lib_root / "src" / "NimBLEDevice.cpp")
    _patch_msys_buf_alloc(lib_root / "src" / "NimBLEDevice.cpp")


def _patch_nimble(*_args, **_kwargs) -> None:
    main()


if env is not None:
    # Run now (when possible) and again right before linking to catch first-build installs.
    _patch_nimble()
    env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", _patch_nimble)
elif __name__ == "__main__":
    _patch_nimble()
