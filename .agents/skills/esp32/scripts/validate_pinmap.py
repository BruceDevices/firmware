#!/usr/bin/env python3
"""ESP32 GPIO pin map validation script.

Reads a JSON pin assignment, validates it against ESP32-specific rules,
and outputs validation results.

Usage:
    echo '{"platform":"esp32s3",...}' | python validate_pinmap.py
    python validate_pinmap.py input.json
    python validate_pinmap.py --format text input.json
"""

import argparse
import json
import sys
from typing import Any, Dict, List, Optional

from platforms import SUPPORTED_VARIANTS, get_platform


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Validate ESP32 GPIO pin assignments against variant-specific rules."
    )
    parser.add_argument(
        "input_file",
        nargs="?",
        default=None,
        help="JSON input file. Reads from stdin if not provided."
    )
    parser.add_argument(
        "--format",
        choices=["json", "text"],
        default="json",
        help="Output format (default: json)."
    )
    return parser.parse_args()


def read_input(input_file: Optional[str]) -> Dict[str, Any]:
    """Read JSON input from file or stdin."""
    try:
        if input_file is not None:
            with open(input_file, "r") as f:
                content = f.read()
        else:
            if sys.stdin.isatty():
                print("Error: No input file specified and stdin is a terminal. "
                      "Pipe JSON input or provide a filename.", file=sys.stderr)
                sys.exit(2)
            content = sys.stdin.read()
        return json.loads(content)
    except (json.JSONDecodeError, FileNotFoundError, OSError) as e:
        print(f"Input error: {e}", file=sys.stderr)
        sys.exit(2)


def validate_input_structure(data: Any) -> None:
    """Validate that the input JSON has required fields.

    Raises:
        ValueError: If input structure is invalid.
    """
    if not isinstance(data, dict):
        raise ValueError("Input must be a JSON object")
    if "pins" not in data or not isinstance(data["pins"], list):
        raise ValueError("'pins' must be a list")


def normalize_input(data: Dict[str, Any]) -> Dict[str, Any]:
    """Apply default values to optional fields."""
    if "platform" not in data:
        data["platform"] = "esp32"
    # If variant is missing, infer from platform.
    # Normalise common separators so "esp32-s3" and "esp32_s3" map to "esp32s3".
    if "variant" not in data:
        platform = str(data.get("platform") or "esp32").lower().strip()
        # Strip separators that users sometimes include (e.g., "esp32-s3" -> "esp32s3")
        normalised = platform.replace("-", "").replace("_", "").replace(" ", "")
        if normalised in SUPPORTED_VARIANTS:
            data["variant"] = normalised
        elif platform in SUPPORTED_VARIANTS:
            data["variant"] = platform
        else:
            # Leave variant absent so get_platform can provide a precise error message.
            data["variant"] = platform
    if "module" not in data:
        data["module"] = "WROOM"
    return data


def main() -> None:
    args = parse_args()
    data = read_input(args.input_file)
    try:
        validate_input_structure(data)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)
    data = normalize_input(data)

    try:
        platform = get_platform(data["platform"], data["variant"], data["module"])
        result = platform.validate(data)

        if args.format == "json":
            print(json.dumps(result.to_dict(), indent=2))
        else:
            status = "VALID" if result.valid else "INVALID"
            print(f"Validation Result: {status}\n")
            if result.errors:
                print(f"Errors ({len(result.errors)}):")
                for e in result.errors:
                    print(f"  [ERROR] {e.get('message')}")
            if result.warnings:
                print(f"\nWarnings ({len(result.warnings)}):")
                for w in result.warnings:
                    print(f"  [WARNING] {w.get('message')}")

        sys.exit(0 if result.valid else 1)
    except ValueError as e:
        print(f"Validation error: {e}", file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
