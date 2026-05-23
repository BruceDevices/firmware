#!/usr/bin/env python3
"""Validate SubGHz advanced FULL registry coverage against vendored Unleashed protocols.

Checks that the FULL protocol list in:
  src/modules/subghz_advanced/subghz_advanced_decoder_adapter.cpp
matches the protocol registry from:
  lib/subghz_unleashed_vendor/src/subghz/protocols/protocol_items.c

By design we currently exclude RAW/BinRAW from the advanced decoder runtime list.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
ADAPTER = ROOT / "src/modules/subghz_advanced/subghz_advanced_decoder_adapter.cpp"
VENDOR_ITEMS = ROOT / "lib/subghz_unleashed_vendor/src/subghz/protocols/protocol_items.c"
INTENTIONAL_EXCLUDE = {"raw", "bin_raw"}


def _extract_array_symbols(content: str, array_name: str) -> list[str]:
    pattern = rf"{re.escape(array_name)}\[\].*?\{{(.*?)\}};"
    match = re.search(pattern, content, re.S)
    if not match:
        raise RuntimeError(f"Unable to find array `{array_name}`")
    body = match.group(1)
    return re.findall(r"&\s*(subghz_protocol_[a-zA-Z0-9_]+)", body)


def _normalize(symbols: Iterable[str]) -> set[str]:
    out = set()
    prefix = "subghz_protocol_"
    for symbol in symbols:
        name = symbol.strip()
        if name.startswith(prefix):
            name = name[len(prefix) :]
        out.add(name)
    return out


def main() -> int:
    adapter_text = ADAPTER.read_text(encoding="utf-8")
    vendor_text = VENDOR_ITEMS.read_text(encoding="utf-8")

    full_symbols = _extract_array_symbols(adapter_text, "kFullProtocols")
    vendor_symbols = _extract_array_symbols(vendor_text, "subghz_protocol_registry_items")

    full_set = _normalize(full_symbols)
    vendor_set = _normalize(vendor_symbols)
    expected_set = vendor_set - INTENTIONAL_EXCLUDE

    missing = sorted(expected_set - full_set)
    extra = sorted(full_set - expected_set)

    print(f"FULL registry entries: {len(full_set)}")
    print(f"Vendor registry entries: {len(vendor_set)}")
    print(f"Intentional exclusions: {', '.join(sorted(INTENTIONAL_EXCLUDE))}")
    print(f"Expected FULL entries: {len(expected_set)}")

    if missing:
        print("\nMissing in FULL registry:")
        for item in missing:
            print(f"  - {item}")

    if extra:
        print("\nExtra in FULL registry:")
        for item in extra:
            print(f"  - {item}")

    if missing or extra:
        return 1

    print("\nCoverage OK: FULL registry is in sync with vendor (excluding RAW/BinRAW).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
