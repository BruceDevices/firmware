"""
Platform abstraction layer for ESP32 GPIO configuration.

Supported Variants:
    - esp32, esp32s2, esp32s3, esp32c3, esp32c6

Supported Modules:
    - WROOM: Standard flash-only modules (GPIO6-11 reserved)
    - WROVER: Modules with PSRAM (GPIO6-11 and GPIO16-17 reserved)
"""

from typing import Optional

from .base import (
    ConflictType,
    GenerationResult,
    PeripheralGroup,
    Pin,
    Platform,
    ValidationResult,
)
from .esp32 import Esp32Platform


# Single authoritative list of supported variants — imported by scripts too.
SUPPORTED_VARIANTS = ["esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6"]

__all__ = [
    "Pin",
    "PeripheralGroup",
    "ConflictType",
    "ValidationResult",
    "GenerationResult",
    "Platform",
    "SUPPORTED_VARIANTS",
    "get_platform",
]


# Registry of platform implementations.
_PLATFORM_REGISTRY: dict = {
    ("esp32", "esp32"): Esp32Platform,
    ("esp32", "esp32s2"): Esp32Platform,
    ("esp32", "esp32s3"): Esp32Platform,
    ("esp32", "esp32c3"): Esp32Platform,
    ("esp32", "esp32c6"): Esp32Platform,
}


def get_platform(
    platform: str,
    variant: Optional[str] = None,
    module: Optional[str] = None
) -> Platform:
    """
    Factory function to instantiate an ESP32 platform implementation.
    """
    platform_lower = str(platform or "esp32").lower().strip()

    # If platform is a recognised variant name (e.g., "esp32s3"), use it as
    # the variant only when no explicit variant was provided.
    if platform_lower in SUPPORTED_VARIANTS and variant is None:
        variant = platform_lower
    elif platform_lower not in SUPPORTED_VARIANTS and variant is None:
        # Unknown platform string and no explicit variant — try platform as variant
        # so the error message names the bad value clearly.
        variant = platform_lower
    # else: explicit variant provided — honour it regardless of platform string.

    if variant is None:
        variant = "esp32"
    else:
        variant = str(variant or "esp32").lower().strip()

    if variant not in SUPPORTED_VARIANTS:
        raise ValueError(f"Unsupported variant '{variant}'. Supported: {', '.join(SUPPORTED_VARIANTS)}")

    if module is None:
        module = "WROOM"
    else:
        module = module.upper().strip()
        # Normalize common aliases
        if module in ("WROOM32", "WROOM-32"):
            module = "WROOM"
        elif module in ("WROVER32", "WROVER-32"):
            module = "WROVER"
        if module not in ("WROOM", "WROVER"):
            raise ValueError(f"Unsupported module '{module}'. Supported: WROOM, WROVER")

    registry_key = ("esp32", variant)
    if registry_key in _PLATFORM_REGISTRY:
        return _PLATFORM_REGISTRY[registry_key](
            name="esp32",
            variant=variant,
            module=module
        )

    raise ValueError(f"Implementation for {variant} not found.")
