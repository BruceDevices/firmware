#!/usr/bin/env python3
"""Build a SubGHz validation matrix from Unleashed unit test fixtures.

This script parses:
- Unleashed unit test definitions (decoder/encoder sample references)
- Bruce advanced CORE/FULL protocol registries
- Vendored protocol symbol -> display-name mapping

It produces a Markdown report that helps track roadmap item:
"Run .sub sample battery and validate identified protocol coverage".
"""

from __future__ import annotations

import argparse
import datetime as _dt
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
UNIT_TEST_C = (
    ROOT
    / "ref/unleashed-firmware/applications/debug/unit_tests/tests/subghz/subghz_test.c"
)
UNIT_TEST_RES = (
    ROOT
    / "ref/unleashed-firmware/applications/debug/unit_tests/resources/unit_tests/subghz"
)
ADAPTER_CPP = ROOT / "src/modules/subghz_advanced/subghz_advanced_decoder_adapter.cpp"
VENDOR_PROTOCOLS = ROOT / "lib/subghz_unleashed_vendor/src/subghz/protocols"
DEFAULT_OUTPUT = ROOT / "SUBGHZ_V1_SAMPLE_MATRIX.md"


@dataclass
class DecoderSample:
    rel_path: str
    macro: str


@dataclass
class EncoderSample:
    rel_path: str


def _extract_array_symbols(content: str, array_name: str) -> List[str]:
    pattern = rf"{re.escape(array_name)}\[\].*?\{{(.*?)\}};"
    match = re.search(pattern, content, re.S)
    if not match:
        raise RuntimeError(f"Unable to find array `{array_name}` in adapter")
    body = match.group(1)
    return re.findall(r"&\s*(subghz_protocol_[a-zA-Z0-9_]+)", body)


def _parse_adapter_symbols() -> Tuple[List[str], List[str]]:
    text = ADAPTER_CPP.read_text(encoding="utf-8")
    core = _extract_array_symbols(text, "kCoreProtocols")
    full = _extract_array_symbols(text, "kFullProtocols")
    return core, full


def _parse_macro_name_values() -> Dict[str, str]:
    out: Dict[str, str] = {}
    for hdr in VENDOR_PROTOCOLS.glob("*.h"):
        text = hdr.read_text(encoding="utf-8", errors="ignore")
        for macro, value in re.findall(
            r'#define\s+([A-Z0-9_]+_NAME)\s+"([^"]+)"', text
        ):
            out[macro] = value
    return out


def _parse_symbol_name_map(
    macro_values: Dict[str, str],
) -> Tuple[Dict[str, str], Dict[str, str]]:
    out: Dict[str, str] = {}
    macro_to_symbol: Dict[str, str] = {}
    for src in VENDOR_PROTOCOLS.glob("*.c"):
        text = src.read_text(encoding="utf-8", errors="ignore")
        for symbol, name_expr in re.findall(
            r"const\s+SubGhzProtocol\s+(subghz_protocol_[a-zA-Z0-9_]+)\s*=\s*\{.*?\.name\s*=\s*([^,\n]+)",
            text,
            re.S,
        ):
            expr = name_expr.strip()
            name = ""
            if expr.startswith('"') and expr.endswith('"'):
                name = expr.strip('"')
            elif expr in macro_values:
                name = macro_values[expr]
                macro_to_symbol[expr] = symbol
            else:
                # Best-effort fallback from symbol.
                name = symbol.removeprefix("subghz_protocol_")
            out[symbol] = name
    return out, macro_to_symbol


def _parse_unit_test_cases() -> Tuple[List[DecoderSample], List[EncoderSample]]:
    text = UNIT_TEST_C.read_text(encoding="utf-8", errors="ignore")

    decoders = [
        DecoderSample(rel_path=path, macro=macro)
        for path, macro in re.findall(
            r"subghz_decoder_test\(\s*EXT_PATH\(\"([^\"]+)\"\)\s*,\s*([A-Z0-9_]+)\s*\)",
            text,
            re.S,
        )
    ]

    encoders = [
        EncoderSample(rel_path=path)
        for path in re.findall(
            r"subghz_encoder_test\(\s*EXT_PATH\(\"([^\"]+)\"\)\s*\)",
            text,
            re.S,
        )
    ]
    return decoders, encoders


def _unit_rel_to_fs_path(rel_path: str) -> Path:
    # rel_path example: unit_tests/subghz/came_raw.sub
    name = Path(rel_path).name
    candidate = UNIT_TEST_RES / name
    if candidate.exists():
        return candidate

    # Case-insensitive fallback.
    wanted = name.lower()
    for p in UNIT_TEST_RES.glob("*.sub"):
        if p.name.lower() == wanted:
            return p
    return candidate


def _parse_sub_fields(path: Path) -> Dict[str, str]:
    fields: Dict[str, str] = {}
    if not path.exists():
        return fields

    for raw_line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line or ":" not in line:
            continue
        key, value = line.split(":", 1)
        fields[key.strip()] = value.strip()
    return fields


def _sample_type(fields: Dict[str, str], filename: str) -> str:
    proto = fields.get("Protocol", "")
    if proto.upper() in {"RAW", "BINRAW"}:
        return "raw"
    if filename.lower().endswith("_raw.sub"):
        return "raw"
    return "key"


def _to_protocol_symbol_from_macro(macro: str) -> str:
    # SUBGHZ_PROTOCOL_X_NAME -> subghz_protocol_x
    token = macro
    token = token.removeprefix("SUBGHZ_PROTOCOL_")
    token = token.removesuffix("_NAME")
    return "subghz_protocol_" + token.lower()


def _md_table(rows: Iterable[List[str]]) -> str:
    rows = list(rows)
    if not rows:
        return ""
    widths = [max(len(r[i]) for r in rows) for i in range(len(rows[0]))]
    out = []
    out.append("| " + " | ".join(r.ljust(widths[i]) for i, r in enumerate(rows[0])) + " |")
    out.append("| " + " | ".join("-" * widths[i] for i in range(len(widths))) + " |")
    for r in rows[1:]:
        out.append("| " + " | ".join(r[i].ljust(widths[i]) for i in range(len(widths))) + " |")
    return "\n".join(out)


def build_report() -> Tuple[str, int]:
    core_symbols, full_symbols = _parse_adapter_symbols()
    macro_values = _parse_macro_name_values()
    symbol_names, macro_to_symbol = _parse_symbol_name_map(macro_values)
    decoder_cases, encoder_cases = _parse_unit_test_cases()

    core_set = set(core_symbols)
    full_set = set(full_symbols)

    missing_files = 0
    unresolved_macros = 0

    dec_rows: List[List[str]] = [
        [
            "Sample",
            "Expected Protocol",
            "Expected In CORE",
            "Expected In FULL",
            "Type",
            "File",
        ]
    ]
    decoder_symbols_seen: set[str] = set()

    for case in decoder_cases:
        path = _unit_rel_to_fs_path(case.rel_path)
        fields = _parse_sub_fields(path)
        stype = _sample_type(fields, path.name)
        symbol = macro_to_symbol.get(case.macro, _to_protocol_symbol_from_macro(case.macro))
        decoder_symbols_seen.add(symbol)
        expected = macro_values.get(case.macro) or symbol_names.get(symbol) or case.macro
        if expected == case.macro:
            unresolved_macros += 1
        in_core = "yes" if symbol in core_set else "no"
        in_full = "yes" if symbol in full_set else "no"
        exists = "ok" if path.exists() else "missing"
        if not path.exists():
            missing_files += 1
        dec_rows.append(
            [
                path.name,
                expected,
                in_core,
                in_full,
                stype,
                exists,
            ]
        )

    enc_rows: List[List[str]] = [["Sample", "Protocol Field", "Type", "File"]]
    for case in encoder_cases:
        path = _unit_rel_to_fs_path(case.rel_path)
        fields = _parse_sub_fields(path)
        proto = fields.get("Protocol", "Unknown")
        stype = _sample_type(fields, path.name)
        exists = "ok" if path.exists() else "missing"
        if not path.exists():
            missing_files += 1
        enc_rows.append([path.name, proto, stype, exists])

    unique_decoder_protocols = sorted({r[1] for r in dec_rows[1:]})
    decoder_count = len(dec_rows) - 1
    encoder_count = len(enc_rows) - 1
    raw_count = sum(1 for r in dec_rows[1:] if r[4] == "raw")
    key_count = sum(1 for r in dec_rows[1:] if r[4] == "key")
    covered_core_symbols = decoder_symbols_seen & core_set
    covered_full_symbols = decoder_symbols_seen & full_set
    missing_core_symbols = sorted(core_set - decoder_symbols_seen)
    missing_full_symbols = sorted(full_set - decoder_symbols_seen)

    status_lines = []
    status_lines.append(f"- Decoder samples parsed: `{decoder_count}`")
    status_lines.append(f"- Encoder samples parsed: `{encoder_count}`")
    status_lines.append(f"- Decoder sample types: `raw={raw_count}`, `key={key_count}`")
    status_lines.append(f"- Unique expected decoder protocols: `{len(unique_decoder_protocols)}`")
    status_lines.append(
        f"- Files missing from fixture directory: `{missing_files}`"
    )
    status_lines.append(
        f"- Unresolved expected protocol macros: `{unresolved_macros}`"
    )
    status_lines.append(
        "- Minimum battery target (>=10 samples): "
        + ("`PASS`" if decoder_count >= 10 else "`FAIL`")
    )
    status_lines.append(
        f"- CORE fixture coverage: `{len(covered_core_symbols)}/{len(core_set)}`"
    )
    status_lines.append(
        f"- FULL fixture coverage: `{len(covered_full_symbols)}/{len(full_set)}`"
    )
    status_lines.append(
        "- CORE fixture completeness gate: "
        + ("`PASS`" if len(covered_core_symbols) == len(core_set) else "`FAIL`")
    )

    today = _dt.date.today().isoformat()
    report = []
    report.append("# SubGHz V1 Sample Validation Matrix")
    report.append("")
    report.append(f"Generated: `{today}`")
    report.append("")
    report.append("## Summary")
    report.extend(status_lines)
    report.append("")
    report.append("## Decoder Samples (from Unleashed unit tests)")
    report.append(_md_table(dec_rows))
    report.append("")
    report.append("## Encoder Samples (from Unleashed unit tests)")
    report.append(_md_table(enc_rows))
    report.append("")
    report.append("## Expected Decoder Protocols")
    report.append("")
    for proto in unique_decoder_protocols:
        report.append(f"- `{proto}`")
    report.append("")
    report.append("## Coverage vs Runtime Registry")
    report.append("")
    report.append(
        f"- CORE covered by decoder fixtures: `{len(covered_core_symbols)}/{len(core_set)}`"
    )
    report.append(
        f"- FULL covered by decoder fixtures: `{len(covered_full_symbols)}/{len(full_set)}`"
    )
    report.append("")
    if missing_core_symbols:
        report.append("### CORE protocols without decoder fixture")
        report.append("")
        for symbol in missing_core_symbols:
            report.append(
                f"- `{symbol_names.get(symbol, symbol.removeprefix('subghz_protocol_'))}`"
            )
        report.append("")
    if missing_full_symbols:
        report.append("### FULL protocols without decoder fixture")
        report.append("")
        for symbol in missing_full_symbols:
            report.append(
                f"- `{symbol_names.get(symbol, symbol.removeprefix('subghz_protocol_'))}`"
            )
        report.append("")

    exit_code = 0
    if missing_files > 0 or decoder_count < 10 or len(covered_core_symbols) != len(core_set):
        exit_code = 1
    return "\n".join(report).rstrip() + "\n", exit_code


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Output markdown report path (default: {DEFAULT_OUTPUT})",
    )
    parser.add_argument(
        "--stdout",
        action="store_true",
        help="Also print report to stdout.",
    )
    args = parser.parse_args()

    report, code = build_report()
    args.output.write_text(report, encoding="utf-8")
    if args.stdout:
        print(report, end="")
    return code


if __name__ == "__main__":
    sys.exit(main())
