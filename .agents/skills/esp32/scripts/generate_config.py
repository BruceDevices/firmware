#!/usr/bin/env python3
"""ESP32 Configuration Generation Script.

Generates initialization code and configuration snippets for ESP-IDF
and Arduino frameworks based on a validated pin map.

Usage:
    echo '{"platform":"esp32",...}' | python generate_config.py --framework arduino
    python generate_config.py input.json --framework espidf
    python generate_config.py input.json --framework espidf --format text
"""

import argparse
import json
import sys

from platforms import get_platform


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate ESP32 boilerplate code.")
    parser.add_argument("input_file", nargs="?", default=None,
                        help="JSON input file. Reads from stdin if not provided.")
    parser.add_argument("--format", choices=["json", "text"], default="json",
                        help="Output format (default: json).")
    parser.add_argument("--framework", choices=["arduino", "espidf"], required=True,
                        help="Target framework for code generation.")
    return parser.parse_args()


def read_input(input_file):
    """Read JSON input from file or stdin."""
    if input_file:
        with open(input_file, "r") as f:
            return json.load(f)
    if sys.stdin.isatty():
        print("Error: No input file specified and stdin is a terminal. "
              "Pipe JSON input or provide a filename.", file=sys.stderr)
        sys.exit(2)
    return json.loads(sys.stdin.read())


def validate_input_structure(data):
    """Validate that the input JSON has required fields.

    Raises:
        ValueError: If input structure is invalid.
    """
    if not isinstance(data, dict):
        raise ValueError("Input must be a JSON object")
    if "pins" not in data or not isinstance(data["pins"], list):
        raise ValueError("'pins' must be a list")


def main() -> None:
    args = parse_args()
    try:
        data = read_input(args.input_file)
        validate_input_structure(data)

        platform = get_platform(
            data.get("platform", "esp32"),
            data.get("variant"),
            data.get("module", "WROOM"),
        )

        # Validate before generating code
        validation = platform.validate(data)
        if not validation.valid:
            print("Validation errors found:", file=sys.stderr)
            for err in validation.errors:
                print(f"  [ERROR] {err.get('message')}", file=sys.stderr)
            sys.exit(1)

        result = platform.generate_config(data)
        result.init_code = platform.generate_code(data, framework=args.framework)

        if args.format == "json":
            print(json.dumps(result.to_dict(), indent=2))
        else:
            print(f"--- Initialization Code ({args.framework}) ---")
            print(result.init_code)
            print("\n--- Wiring Notes ---")
            for note in result.wiring_notes:
                print(f"- {note}")
            if result.warnings:
                print("\n--- Warnings ---")
                for w in result.warnings:
                    print(f"  [WARNING] {w}")
            if validation.warnings:
                print("\n--- Validation Warnings ---")
                for w in validation.warnings:
                    print(f"  [WARNING] {w.get('message')}")

    except (json.JSONDecodeError, FileNotFoundError, OSError) as e:
        print(f"Input error: {e}", file=sys.stderr)
        sys.exit(2)
    except ValueError as e:
        print(f"Validation error: {e}", file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
