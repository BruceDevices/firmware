"""
ESP32 platform implementation for GPIO configuration.

This module provides the concrete Esp32Platform class that implements the
Platform ABC for ESP32 GPIO validation and code generation.

Supports variants: esp32, esp32s2, esp32s3, esp32c3, esp32c6
Supports modules: WROOM (flash only), WROVER (with PSRAM)

Python 3.9+ compatible. Uses only standard library modules.
"""

import re
from typing import Any, Dict, List, Optional, Set

from .base import (
    ConflictType,
    GenerationResult,
    PeripheralGroup,
    Pin,
    Platform,
    ValidationResult,
)


# Flash SPI pins per variant (see _get_flash_pins())
_FLASH_PINS_ESP32: Set[int] = {6, 7, 8, 9, 10, 11}
_FLASH_PINS_ESP32C3: Set[int] = {12, 13, 14, 15, 16, 17}
_FLASH_PINS_ESP32C6: Set[int] = {24, 25, 26, 27, 28, 29}
_FLASH_PINS_ESP32S2: Set[int] = {26, 27, 28, 29, 30, 31, 32}
_FLASH_PINS_ESP32S3: Set[int] = {26, 27, 28, 29, 30, 31, 32}

# Strapping pins per variant (see _get_strapping_pin_set())
_STRAPPING_PINS_ESP32_SET: Set[int] = {0, 2, 5, 12, 15}
_STRAPPING_PINS_ESP32S2_SET: Set[int] = {0, 45, 46}
_STRAPPING_PINS_ESP32S3_SET: Set[int] = {0, 3, 45, 46}
_STRAPPING_PINS_ESP32C3_SET: Set[int] = {2, 8, 9}
_STRAPPING_PINS_ESP32C6_SET: Set[int] = {4, 5, 8, 9, 15}

# Per-variant GPIO count (max GPIO number + 1)
_GPIO_COUNT: Dict[str, int] = {
    "esp32": 40,     # GPIO0-39
    "esp32s2": 47,   # GPIO0-46
    "esp32s3": 49,   # GPIO0-48
    "esp32c3": 22,   # GPIO0-21
    "esp32c6": 31,   # GPIO0-30
}

# Per-variant ADC2 GPIOs (conflict with WiFi)
_ADC2_GPIOS: Dict[str, Set[int]] = {
    "esp32": {0, 2, 4, 12, 13, 14, 15, 25, 26, 27},
    "esp32s2": {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
    "esp32s3": {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
    "esp32c3": set(),   # C3 has single ADC unit, no WiFi conflict
    "esp32c6": set(),   # C6 has single ADC unit, no WiFi conflict
}

# Per-variant PSRAM pins (reserved on WROVER modules only)
_PSRAM_PINS: Dict[str, Set[int]] = {
    "esp32": {16, 17},
    "esp32s2": set(),   # PSRAM uses dedicated pins on S2
    "esp32s3": set(),   # PSRAM uses dedicated pins on S3
    "esp32c3": set(),   # No PSRAM support
    "esp32c6": set(),   # No PSRAM support
}

# Per-variant input-only pins (no output, no internal pulls)
_INPUT_ONLY_PINS: Dict[str, Set[int]] = {
    "esp32": {34, 35, 36, 37, 38, 39},
    "esp32s2": {46},    # GPIO46 is input-only on S2
    "esp32s3": {46},    # GPIO46 is input-only on S3
    "esp32c3": set(),   # All GPIO bidirectional on C3
    "esp32c6": set(),   # All GPIO bidirectional on C6
}

# Per-module non-exposed pins (exist in silicon but not bonded out on the module)
_NON_EXPOSED_PINS: Dict[str, Set[int]] = {
    "WROOM": {20, 24, 28, 29, 30, 31, 37, 38},   # Not bonded on WROOM-32
    "WROVER": {20, 24, 28, 29, 30, 31, 37, 38},  # Same for WROVER-32
}


class Esp32Platform(Platform):
    """
    Concrete Platform implementation for ESP32 GPIO.

    Provides complete GPIO pin database for all 40 GPIO pins (GPIO0-GPIO39),
    peripheral group definitions for SPI, I2C, and UART, and validation
    logic for detecting conflicts, reserved pins, and electrical constraints.

    Supports ESP32, ESP32-S2, ESP32-S3, ESP32-C3, and ESP32-C6 variants.
    Supports WROOM (flash only) and WROVER (with PSRAM) modules.

    Key ESP32-specific constraints:
    - GPIO6-11: Flash SPI pins - NEVER use
    - GPIO16-17: PSRAM on WROVER modules - reserved if WROVER
    - GPIO34-39: Input-only pins - no output, no internal pulls
    - GPIO0, 2, 5, 12, 15: Strapping pins - restricted with warnings
    - ADC2 (GPIO0,2,4,12-15,25-27): Unusable when WiFi active

    Attributes:
        name: Always "esp32" for ESP32 family.
        variant: One of "esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6".
        module: One of "WROOM" or "WROVER".

    Example:
        >>> platform = Esp32Platform("esp32", "esp32", "WROOM")
        >>> pin = platform.get_pin(21)
        >>> print(pin.usability)
        free
        >>> result = platform.validate(assignment_dict)
        >>> print(result.valid)
        True
    """

    def __init__(
        self,
        name: str,
        variant: str,
        module: Optional[str] = None
    ) -> None:
        """
        Initialize the ESP32 platform.

        Args:
            name: Platform identifier, should be "esp32".
            variant: ESP32 variant - "esp32", "esp32s2", "esp32s3", "esp32c3", or "esp32c6".
            module: Module type - "WROOM" or "WROVER". Defaults to "WROOM".
        """
        if module is None:
            module = "WROOM"
        super().__init__(name, variant, module)
        self._groups = self._build_peripheral_groups()
        self._pins = self._build_pins()

    def _build_pins(self) -> Dict[int, Pin]:
        """Build the complete pin database for the current ESP32 variant."""
        pins = {}
        gpio_count = _GPIO_COUNT.get(self.variant, 40)

        for gpio_num in range(gpio_count):
            pins[gpio_num] = self._create_pin(gpio_num)

        return pins

    def _get_input_only_pins(self) -> Set[int]:
        """Get input-only pins for the current variant."""
        return _INPUT_ONLY_PINS.get(self.variant, _INPUT_ONLY_PINS["esp32"])

    def _get_psram_pins(self) -> Set[int]:
        """Get PSRAM-reserved pins for the current variant."""
        return _PSRAM_PINS.get(self.variant, _PSRAM_PINS["esp32"])

    def _get_adc2_gpios(self) -> Set[int]:
        """Get ADC2 GPIOs for the current variant."""
        return _ADC2_GPIOS.get(self.variant, _ADC2_GPIOS["esp32"])

    def _get_gpio_count(self) -> int:
        """Get the number of GPIOs for the current variant."""
        return _GPIO_COUNT.get(self.variant, 40)

    def _get_non_exposed_pins(self) -> Set[int]:
        """Get pins not bonded out on the current module (base ESP32 only)."""
        if self.variant == "esp32":
            return _NON_EXPOSED_PINS.get(self.module or "WROOM", set())
        return set()

    def _create_pin(self, gpio_num: int) -> Pin:
        """Create a Pin object for a specific GPIO number."""
        # Determine usability and reason
        usability, usability_reason = self._determine_usability(gpio_num)

        # Determine capabilities based on pin type
        input_only_pins = self._get_input_only_pins()
        psram_pins = self._get_psram_pins()
        non_exposed = self._get_non_exposed_pins()
        is_input_only = gpio_num in input_only_pins
        flash_pins = self._get_flash_pins()
        is_reserved = (gpio_num in flash_pins
                       or (gpio_num in psram_pins and self.module == "WROVER")
                       or gpio_num in non_exposed)
        can_output = not is_input_only and not is_reserved
        can_input = not is_reserved

        # Internal pulls - input-only pins have no internal pull resistors
        if is_input_only:
            internal_pulls = ["none"]
        else:
            internal_pulls = ["up", "down"]

        # Default state
        default_state = self._get_default_state(gpio_num)

        # ADC channel
        adc_channel = self._get_adc_channel(gpio_num)

        # DAC channel (only GPIO25 and GPIO26 on original ESP32)
        dac_channel = self._get_dac_channel(gpio_num)

        # Touch channel
        touch_channel = self._get_touch_channel(gpio_num)

        # PWM - all output-capable pins can do PWM via LEDC
        pwm_capable = can_output

        # Alternate functions (ESP32 uses GPIO matrix, so these are conventions)
        alternate_functions = self._get_alternate_functions(gpio_num)

        # Notes
        notes = self._get_notes(gpio_num)

        # Conflicts - base conflicts for pin type
        conflicts = self._get_base_conflicts(gpio_num)

        return Pin(
            gpio_num=gpio_num,
            name=f"GPIO{gpio_num}",
            physical_pin=None,  # ESP32 modules vary in pinout
            voltage=3.3,
            max_current_ma=40.0,  # Absolute max, 20mA recommended
            internal_pulls=internal_pulls,
            default_state=default_state,
            alternate_functions=alternate_functions,
            can_input=can_input,
            can_output=can_output,
            adc_channel=adc_channel,
            dac_channel=dac_channel,
            touch_channel=touch_channel,
            pwm_capable=pwm_capable,
            usability=usability,
            usability_reason=usability_reason,
            conflicts=conflicts,
            notes=notes
        )

    def _determine_usability(self, gpio_num: int) -> tuple:
        """Determine usability status and reason for a GPIO."""
        # Non-exposed pins - not bonded out on this module
        non_exposed = self._get_non_exposed_pins()
        if gpio_num in non_exposed:
            return ("unusable", f"GPIO{gpio_num} is not exposed on {self.module} module")

        # Flash pins - reserved (system use) - variant-specific
        flash_pins = self._get_flash_pins()
        if gpio_num in flash_pins:
            return ("reserved", "Flash SPI bus - never use")

        # PSRAM pins - reserved on WROVER, free on WROOM
        psram_pins = self._get_psram_pins()
        if gpio_num in psram_pins:
            if self.module == "WROVER":
                return ("reserved", "PSRAM SPI on WROVER modules")
            # On WROOM, these are free
            return ("free", None)

        # Input-only pins - restricted (limited capability)
        input_only_pins = self._get_input_only_pins()
        if gpio_num in input_only_pins:
            return ("restricted", "Input-only - no output, no internal pulls")

        # Strapping pins - restricted with boot-mode implications (variant-specific)
        strapping_pins = self._get_strapping_pins()
        if gpio_num in strapping_pins:
            msg = strapping_pins.get(gpio_num, "Strapping pin with boot implications")
            return ("restricted", f"Strapping pin - {msg}")

        # All other pins are free
        return ("free", None)

    def _get_default_state(self, gpio_num: int) -> str:
        """Get the default boot state for a GPIO (variant-aware)."""
        strapping_pins = self._get_strapping_pin_set()

        # Only apply boot-state overrides for actual strapping pins on this variant.
        # Values conform to the Pin.default_state enum: "input_pullup", "input_pulldown",
        # "input_floating", "output_low".
        if gpio_num in strapping_pins:
            variant_lower = self.variant.lower()
            if variant_lower == "esp32":
                boot_states = {0: "input_pullup", 2: "input_pulldown", 5: "input_pullup",
                              12: "input_pulldown", 15: "input_pullup"}
            elif "s2" in variant_lower:
                boot_states = {0: "input_pullup", 45: "input_pulldown", 46: "input_pulldown"}
            elif "s3" in variant_lower:
                boot_states = {0: "input_pullup", 3: "input_pullup", 45: "input_pulldown", 46: "input_pulldown"}
            elif "c3" in variant_lower:
                boot_states = {2: "input_pullup", 8: "input_pulldown", 9: "input_pullup"}
            elif "c6" in variant_lower:
                boot_states = {4: "input_pulldown", 5: "input_pulldown", 8: "input_pulldown",
                              9: "input_pullup", 15: "input_pulldown"}
            else:
                boot_states = {}
            if gpio_num in boot_states:
                return boot_states[gpio_num]

        return "input_floating"

    def _get_adc_channel(self, gpio_num: int) -> Optional[str]:
        """Get ADC channel for a GPIO if available (variant-aware)."""
        return self._get_adc_info(gpio_num)

    def _get_dac_channel(self, gpio_num: int) -> Optional[str]:
        """Get DAC channel for a GPIO if available."""
        # Only original ESP32 has DAC on GPIO25 and GPIO26
        if self.variant == "esp32":
            if gpio_num == 25:
                return "DAC1"
            if gpio_num == 26:
                return "DAC2"
        return None

    def _get_touch_channel(self, gpio_num: int) -> Optional[str]:
        """Get touch channel for a GPIO if available (variant-aware)."""
        variant_lower = self.variant.lower()
        if variant_lower == "esp32":
            touch_map = {
                4: "TOUCH0", 0: "TOUCH1", 2: "TOUCH2", 15: "TOUCH3",
                13: "TOUCH4", 12: "TOUCH5", 14: "TOUCH6", 27: "TOUCH7",
                33: "TOUCH8", 32: "TOUCH9",
            }
        elif "s2" in variant_lower or "s3" in variant_lower:
            # ESP32-S2/S3: Touch on GPIO1-14
            touch_map = {i: f"TOUCH{i}" for i in range(1, 15)}
        else:
            # ESP32-C3/C6 do not have touch capability
            touch_map = {}
        return touch_map.get(gpio_num)

    def _get_alternate_functions(self, gpio_num: int) -> Dict[str, str]:
        """Get conventional alternate functions for a GPIO (variant-aware)."""
        alt_funcs = {}

        # Get peripheral groups for this variant and map pin -> function
        for group in self._groups.values():
            for signal_name, pin_num in group.pins.items():
                if pin_num == gpio_num:
                    alt_funcs[f"{group.name}_{signal_name}"] = f"{group.name} {signal_name}"

        return alt_funcs

    def _get_notes(self, gpio_num: int) -> str:
        """Get notes for a GPIO."""
        notes = []

        flash_pins = self._get_flash_pins()
        if gpio_num in flash_pins:
            notes.append("Flash SPI - DO NOT USE")

        psram_pins = self._get_psram_pins()
        if gpio_num in psram_pins:
            if self.module == "WROVER":
                notes.append("PSRAM SPI on WROVER - DO NOT USE")

        input_only_pins = self._get_input_only_pins()
        if gpio_num in input_only_pins:
            notes.append("Input-only, no internal pulls")

        strapping_pins = self._get_strapping_pins()
        if gpio_num in strapping_pins:
            notes.append(f"Strapping pin - {strapping_pins[gpio_num]}")

        adc2_gpios = self._get_adc2_gpios()
        if gpio_num in adc2_gpios:
            notes.append("ADC2 - unavailable when WiFi active")

        uart0_group = self._groups.get("UART0")
        if uart0_group and gpio_num in uart0_group.pins.values():
            signal = [k for k, v in uart0_group.pins.items() if v == gpio_num][0]
            notes.append(f"UART0 {signal} - connected to USB serial")

        return "; ".join(notes) if notes else ""

    def _get_base_conflicts(self, gpio_num: int) -> List[str]:
        """Get base conflicts for a GPIO."""
        conflicts = []

        flash_pins = self._get_flash_pins()
        if gpio_num in flash_pins:
            conflicts.append("FLASH_SPI")

        psram_pins = self._get_psram_pins()
        if gpio_num in psram_pins and self.module == "WROVER":
            conflicts.append("PSRAM_SPI")

        uart0_group = self._groups.get("UART0")
        if uart0_group and gpio_num in uart0_group.pins.values():
            conflicts.append("UART0_USB")

        return conflicts

    def _build_peripheral_groups(self) -> Dict[str, PeripheralGroup]:
        """Build the peripheral group definitions (variant-aware)."""
        groups = {}
        variant_lower = self.variant.lower()

        if variant_lower == "esp32":
            # Base ESP32 peripheral groups
            groups["VSPI"] = PeripheralGroup(
                name="VSPI", protocol="spi",
                pins={"MOSI": 23, "MISO": 19, "SCLK": 18, "CS0": 5},
                is_fixed=False, enabled_by=None,
                notes="Recommended SPI bus. CS0 is strapping pin."
            )
            groups["HSPI"] = PeripheralGroup(
                name="HSPI", protocol="spi",
                pins={"MOSI": 13, "MISO": 12, "SCLK": 14, "CS0": 15},
                is_fixed=False, enabled_by=None,
                notes="GPIO12/15 are strapping pins - use with caution"
            )
            groups["I2C"] = PeripheralGroup(
                name="I2C", protocol="i2c",
                pins={"SDA": 21, "SCL": 22},
                is_fixed=False, enabled_by=None,
                notes="Conventional pins; any GPIO works via GPIO matrix"
            )
            groups["UART2"] = PeripheralGroup(
                name="UART2", protocol="uart",
                pins={"TX": 17, "RX": 16},
                is_fixed=False, enabled_by=None,
                notes="GPIO16/17 unavailable on WROVER (PSRAM)"
            )
        elif "s3" in variant_lower or "s2" in variant_lower:
            # ESP32-S2/S3 use SPI2/SPI3 naming
            groups["SPI2"] = PeripheralGroup(
                name="SPI2", protocol="spi",
                pins={"MOSI": 11, "MISO": 13, "SCLK": 12, "CS0": 10},
                is_fixed=False, enabled_by=None,
                notes="Default SPI2 pins on S2/S3. All remappable via GPIO matrix."
            )
            groups["I2C"] = PeripheralGroup(
                name="I2C", protocol="i2c",
                pins={"SDA": 8, "SCL": 9},
                is_fixed=False, enabled_by=None,
                notes="Conventional pins on S2/S3; any GPIO works via GPIO matrix"
            )
        elif "c3" in variant_lower:
            # ESP32-C3 (GPIO0-21)
            groups["SPI2"] = PeripheralGroup(
                name="SPI2", protocol="spi",
                pins={"MOSI": 7, "MISO": 2, "SCLK": 6, "CS0": 10},
                is_fixed=False, enabled_by=None,
                notes="Default SPI2 pins on C3. All remappable."
            )
            groups["I2C"] = PeripheralGroup(
                name="I2C", protocol="i2c",
                pins={"SDA": 8, "SCL": 9},
                is_fixed=False, enabled_by=None,
                notes="Conventional pins on C3; any GPIO works via GPIO matrix"
            )
        elif "c6" in variant_lower:
            # ESP32-C6 (GPIO0-30)
            groups["SPI2"] = PeripheralGroup(
                name="SPI2", protocol="spi",
                pins={"MOSI": 7, "MISO": 2, "SCLK": 6, "CS0": 10},
                is_fixed=False, enabled_by=None,
                notes="Default SPI2 pins on C6. All remappable."
            )
            groups["I2C"] = PeripheralGroup(
                name="I2C", protocol="i2c",
                pins={"SDA": 21, "SCL": 22},
                is_fixed=False, enabled_by=None,
                notes="Conventional pins on C6; any GPIO works via GPIO matrix"
            )

        # UART0 - variant-specific USB serial pins
        if variant_lower == "esp32":
            uart0_pins = {"TX": 1, "RX": 3}
        elif "s2" in variant_lower or "s3" in variant_lower:
            uart0_pins = {"TX": 43, "RX": 44}
        elif "c6" in variant_lower:
            uart0_pins = {"TX": 16, "RX": 17}
        else:  # c3
            uart0_pins = {"TX": 21, "RX": 20}
        groups["UART0"] = PeripheralGroup(
            name="UART0", protocol="uart",
            pins=uart0_pins,
            is_fixed=True, enabled_by=None,
            notes="Connected to USB serial - avoid for peripherals"
        )

        return groups

    def get_pin(self, gpio_num: int) -> Pin:
        """
        Retrieve a specific pin by its GPIO number.

        Args:
            gpio_num: GPIO number (range depends on variant: 0-39 for ESP32,
                0-48 for ESP32-S3, 0-21 for ESP32-C3, etc.).

        Returns:
            Pin object for the requested GPIO.

        Raises:
            ValueError: If gpio_num is not in the valid range for this variant.
        """
        if gpio_num not in self._pins:
            max_gpio = self._get_gpio_count() - 1
            raise ValueError(
                f"Invalid GPIO number {gpio_num} for {self.variant}. "
                f"Valid range: 0-{max_gpio}."
            )
        return self._pins[gpio_num]

    def get_all_pins(self) -> List[Pin]:
        """
        Retrieve all GPIO pins for this variant, sorted by GPIO number.

        Returns:
            List of all Pin objects, ordered by gpio_num ascending.
            Count varies by variant (ESP32: 40, S3: 49, C3: 22, C6: 31).
        """
        return [self._pins[i] for i in sorted(self._pins.keys())]

    def get_peripheral_group(self, name: str) -> PeripheralGroup:
        """
        Retrieve a peripheral group by name.

        Args:
            name: Peripheral group name (case-insensitive).

        Returns:
            PeripheralGroup object.

        Raises:
            ValueError: If no group with the given name exists.
        """
        name_upper = name.upper()
        if name_upper not in self._groups:
            available = ", ".join(sorted(self._groups.keys()))
            raise ValueError(
                f"Unknown peripheral group '{name}'. "
                f"Available groups: {available}."
            )
        return self._groups[name_upper]

    def get_all_peripheral_groups(self) -> List[PeripheralGroup]:
        """
        Retrieve all peripheral groups.

        Returns:
            List of all PeripheralGroup objects.
        """
        return list(self._groups.values())

    def get_groups_for_protocol(self, protocol: str) -> List[PeripheralGroup]:
        """
        Retrieve all peripheral groups that use a specific protocol.

        Args:
            protocol: Protocol identifier (e.g., "spi", "i2c", "uart").

        Returns:
            List of PeripheralGroup objects matching the protocol.
        """
        protocol_lower = protocol.lower()
        return [g for g in self._groups.values() if g.protocol == protocol_lower]

    def validate(self, assignment: Dict[str, Any]) -> ValidationResult:
        """
        Validate a proposed GPIO pin assignment configuration.

        Runs validation checks in order:
        1. FLASH_PIN - GPIO6-11 cannot be assigned (always error)
        2. DIRECT_CONFLICT - Same GPIO assigned multiple times
        3. INPUT_ONLY - Output requested on GPIO34-39
        4. ADC2_WIFI - ADC2 pin used when WiFi enabled
        5. STRAPPING_PIN - Strapping pin used (warning)
        6. PSRAM_PIN - GPIO16-17 on WROVER module
        7. BOOT_STATE - GPIO12 HIGH at boot warning
        8. MISSING_PULLUP - I2C/1-Wire needs external pull-ups
        9. ELECTRICAL_CURRENT - Total current draw check

        Args:
            assignment: Pin assignment dict with keys: platform, variant,
                module, wifi_enabled, pins (list of pin assignments).

        Returns:
            ValidationResult with valid flag, errors, warnings, and summary.
        """
        errors = []  # type: List[Dict[str, Any]]
        warnings = []  # type: List[Dict[str, Any]]
        pins_list = assignment.get("pins", [])
        wifi_enabled = assignment.get("wifi_enabled", False)
        module = assignment.get("module", self.module)

        # Normalize module name
        if module is not None:
            module = module.upper()
            if module in ("WROOM32", "WROOM-32"):
                module = "WROOM"
            elif module in ("WROVER32", "WROVER-32"):
                module = "WROVER"
            elif module not in ("WROOM", "WROVER"):
                warnings.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": f"Unknown module '{module}'. Defaulting to WROOM. "
                               f"Supported modules: WROOM, WROVER.",
                    "severity": "warning"
                })
                module = "WROOM"

        # Track seen GPIOs for conflict detection: gpio -> (function, protocol_bus)
        seen_gpios = {}  # type: Dict[int, tuple]

        # Get variant-specific pin sets
        flash_pins = self._get_flash_pins()
        input_only_pins = self._get_input_only_pins()
        psram_pins = self._get_psram_pins()
        adc2_gpios = self._get_adc2_gpios()
        non_exposed = self._get_non_exposed_pins()
        strapping_pin_set = self._get_strapping_pin_set()
        strapping_msgs = self._get_strapping_pins()
        max_gpio = self._get_gpio_count() - 1

        for pin_assignment in pins_list:
            if not isinstance(pin_assignment, dict):
                errors.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": "Pin assignment must be a JSON object, "
                               f"got {type(pin_assignment).__name__}.",
                    "severity": "error"
                })
                continue
            raw_gpio = pin_assignment.get("gpio")
            # Coerce to int and validate
            if raw_gpio is None:
                errors.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": "Pin assignment missing required 'gpio' field.",
                    "severity": "error"
                })
                continue
            if isinstance(raw_gpio, bool):
                errors.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": f"Invalid gpio value '{raw_gpio}': must be an integer, not boolean.",
                    "severity": "error"
                })
                continue
            # Reject floats with fractional parts
            if isinstance(raw_gpio, float) and raw_gpio != int(raw_gpio):
                errors.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": f"Invalid gpio value '{raw_gpio}': must be an integer, not float.",
                    "severity": "error"
                })
                continue
            try:
                gpio = int(raw_gpio)
            except (TypeError, ValueError):
                errors.append({
                    "gpio": -1,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": f"Invalid gpio value '{raw_gpio}': must be an integer.",
                    "severity": "error"
                })
                continue

            # Check 0: GPIO range
            if gpio < 0 or gpio > max_gpio:
                errors.append({
                    "gpio": gpio,
                    "code": ConflictType.INVALID_GPIO.value,
                    "message": f"GPIO{gpio} is out of range for {self.variant}. "
                               f"Valid range: 0-{max_gpio}.",
                    "severity": "error"
                })
                continue

            # Check 0b: Non-exposed pins (error - physically unbonded)
            if gpio in non_exposed:
                errors.append({
                    "gpio": gpio,
                    "code": ConflictType.RESERVED_PIN.value,
                    "message": f"GPIO{gpio} is not exposed on {self.module} module "
                               f"and cannot be physically wired.",
                    "severity": "error"
                })
                continue

            function = str(pin_assignment.get("function") or "")
            protocol_bus = str(pin_assignment.get("protocol_bus") or "")
            direction = str(pin_assignment.get("direction") or "")
            pull = str(pin_assignment.get("pull") or "none")
            device = str(pin_assignment.get("device") or "device")

            # Check 1: FLASH_PIN (error - variant-specific)
            if gpio in flash_pins:
                errors.append({
                    "gpio": gpio,
                    "code": ConflictType.FLASH_PIN.value,
                    "message": f"GPIO{gpio} is a flash SPI pin on {self.variant}. "
                               f"Cannot assign user functions.",
                    "severity": "error"
                })
                continue

            # Check 2: DIRECT_CONFLICT (bus protocols like I2C/SPI share pins)
            if gpio in seen_gpios:
                prev_func, prev_proto = seen_gpios[gpio]
                bus_protocols = ("i2c", "spi", "1wire", "onewire")
                is_bus_share = (
                    protocol_bus.lower() in bus_protocols
                    and prev_proto.lower() in bus_protocols
                    and prev_func.lower() == function.lower()
                )
                if not is_bus_share:
                    errors.append({
                        "gpio": gpio,
                        "code": ConflictType.DIRECT_CONFLICT.value,
                        "message": f"GPIO{gpio} is assigned multiple times: "
                                   f"'{prev_func}' and '{function}'.",
                        "severity": "error"
                    })
            else:
                seen_gpios[gpio] = (function, protocol_bus)

            # Check 3: INPUT_ONLY
            if gpio in input_only_pins:
                if self._is_output_function(function, protocol_bus, direction):
                    errors.append({
                        "gpio": gpio,
                        "code": ConflictType.INPUT_ONLY.value,
                        "message": f"GPIO{gpio} is input-only on {self.variant}. Cannot use for "
                                   f"output function '{function}'.",
                        "severity": "error"
                    })
                # Bidirectional buses (I2C SDA, 1-Wire) require open-drain drive
                bidirectional_protocols = ("i2c", "onewire", "1wire")
                if protocol_bus.lower() in bidirectional_protocols:
                    errors.append({
                        "gpio": gpio,
                        "code": ConflictType.INPUT_ONLY.value,
                        "message": f"GPIO{gpio} is input-only on {self.variant}. "
                                   f"{protocol_bus.upper()} requires bidirectional (open-drain) drive "
                                   f"which input-only pins cannot provide.",
                        "severity": "error"
                    })
                if pull in ("internal_up", "up", "internal_down", "down"):
                    warnings.append({
                        "gpio": gpio,
                        "code": ConflictType.INPUT_ONLY.value,
                        "message": f"GPIO{gpio} is input-only and has no internal pull resistors. "
                                   f"Use an external pull resistor instead of '{pull}'.",
                        "severity": "warning"
                    })

            # Check 4: ADC2_WIFI
            if gpio in adc2_gpios and wifi_enabled:
                is_adc_use = (
                    "adc" in function.lower() or
                    "analog" in function.lower() or
                    protocol_bus.lower() == "adc"
                )
                if is_adc_use:
                    errors.append({
                        "gpio": gpio,
                        "code": ConflictType.ADC2_WIFI.value,
                        "message": f"GPIO{gpio} (ADC2) is unavailable on {self.variant} when WiFi "
                                   f"is active. Use ADC1 pins instead.",
                        "severity": "error"
                    })

            # Check 5: STRAPPING_PIN (warning - variant-specific)
            if gpio in strapping_pin_set:
                msg = strapping_msgs.get(
                    gpio,
                    f"Strapping pin on {self.variant} with boot implications."
                )
                warnings.append({
                    "gpio": gpio,
                    "code": ConflictType.STRAPPING_PIN.value,
                    "message": f"GPIO{gpio} is a strapping pin. {msg}",
                    "severity": "warning"
                })

            # Check 5b: UART0 USB serial pin warning
            uart0_group = self._groups.get("UART0")
            if uart0_group and gpio in uart0_group.pins.values():
                signal = [k for k, v in uart0_group.pins.items() if v == gpio][0]
                warnings.append({
                    "gpio": gpio,
                    "code": ConflictType.PERIPHERAL_GROUP.value,
                    "message": f"GPIO{gpio} is the UART0 {signal} pin (USB serial) on {self.variant}. "
                               f"Using it for peripherals may interfere with flashing and serial monitor.",
                    "severity": "warning"
                })

            # Check 6: PSRAM_PIN
            if gpio in psram_pins and module == "WROVER":
                errors.append({
                    "gpio": gpio,
                    "code": ConflictType.PSRAM_PIN.value,
                    "message": f"GPIO{gpio} is used for PSRAM on WROVER modules. "
                               f"Cannot assign user functions.",
                    "severity": "error"
                })

            # Check 7: BOOT_STATE - flash voltage pins with elevated risk
            # GPIO12 on base ESP32, GPIO45 on S2/S3
            variant_lower = self.variant.lower()
            flash_voltage_pin = None
            if variant_lower == "esp32" and gpio == 12:
                flash_voltage_pin = 12
            elif ("s2" in variant_lower or "s3" in variant_lower) and gpio == 45:
                flash_voltage_pin = 45

            if flash_voltage_pin is not None:
                if pull in ("up", "external_up", "internal_up") or "high" in function.lower():
                    warnings.append({
                        "gpio": gpio,
                        "code": ConflictType.STRAPPING_PIN.value,
                        "message": f"GPIO{gpio} with pull-up or HIGH output at boot "
                                   f"can set wrong flash voltage and damage the module. "
                                   f"Ensure external circuitry keeps it LOW at boot.",
                        "severity": "warning"
                    })

            # Check 8: MISSING_PULLUP
            if protocol_bus.lower() in ("i2c", "onewire", "1wire"):
                if pull != "external_up":
                    warnings.append({
                        "gpio": gpio,
                        "code": ConflictType.MISSING_PULLUP.value,
                        "message": f"GPIO{gpio} ({protocol_bus}) requires external "
                                   f"pull-up (4.7k recommended). Internal ~45k is "
                                   f"too weak.",
                        "severity": "warning"
                    })

        # Check 9: PWM channel count (variant-aware)
        # Guard: only count dict entries — non-dicts were rejected with 'continue' above
        pwm_count = sum(
            1 for p in pins_list
            if isinstance(p, dict)
            and (str(p.get("protocol_bus") or "").lower() == "pwm"
                 or "PWM" in str(p.get("function") or "").upper())
        )
        variant_lower = self.variant.lower()
        max_pwm = 6 if ("c3" in variant_lower or "c6" in variant_lower) else 16
        if pwm_count > max_pwm:
            warnings.append({
                "gpio": -1,
                "code": ConflictType.RESOURCE_CONFLICT.value,
                "message": f"{pwm_count} PWM pins requested but {self.variant} supports "
                           f"max {max_pwm} LEDC channels. Only the first {max_pwm} will be configured.",
                "severity": "warning"
            })

        # Check 10: Current estimation
        estimated_current = self._estimate_current(pins_list)

        if estimated_current > 200:
            errors.append({
                "gpio": -1,
                "code": ConflictType.ELECTRICAL_CURRENT.value,
                "message": f"Estimated total GPIO current {estimated_current}mA "
                           f"exceeds ESP32 safe limit of 200mA.",
                "severity": "error"
            })
        elif estimated_current > 160:
            warnings.append({
                "gpio": -1,
                "code": ConflictType.ELECTRICAL_CURRENT.value,
                "message": f"Estimated GPIO current {estimated_current}mA is "
                           f"approaching the 200mA limit.",
                "severity": "warning"
            })

        return ValidationResult(
            valid=len(errors) == 0,
            errors=errors,
            warnings=warnings,
            summary={
                "total_pins": len(pins_list),
                "errors": len(errors),
                "warnings": len(warnings),
                "current_draw_ma": estimated_current
            }
        )

    def _is_output_function(
        self, function: str, protocol_bus: str, direction: str = ""
    ) -> bool:
        """Check if a function implies output capability.

        Uses explicit ``direction`` field first when available, then falls
        back to keyword heuristics on ``function`` and ``protocol_bus``.
        """
        # 1. Honour explicit direction when provided
        dir_lower = (direction or "").lower().strip()
        if dir_lower in ("output", "out", "inout"):
            return True
        if dir_lower in ("input", "in"):
            return False

        # 2. Heuristic fallback
        func_lower = (function or "").lower()
        proto_lower = (protocol_bus or "").lower()

        # Tokenize on non-alphanumeric boundaries for word-level matching
        tokens = set(re.split(r'[^a-z0-9]+', func_lower))

        # Long keywords safe for substring matching
        output_substr = ["output", "led", "relay", "motor", "mosi", "sclk",
                        "dout", "reset", "enable"]
        input_substr = ["miso", "din", "input", "analog_in"]

        # Short keywords require exact token match to avoid false positives
        # (e.g., "rx" in "matrix", "en" in "screen", "cs" in "physics")
        output_tokens = {"tx", "cs", "ss", "en", "rst", "clk", "pwm"}
        input_tokens = {"rx", "adc"}

        # Check input signals first
        for keyword in input_substr:
            if keyword in func_lower:
                return False
        if tokens & input_tokens:
            return False

        # Check output signals
        for keyword in output_substr:
            if keyword in func_lower:
                return True
        if tokens & output_tokens:
            return True

        # Protocol-based detection
        if proto_lower == "pwm":
            return True
        if proto_lower == "gpio" and "out" in func_lower:
            return True

        return False

    def _estimate_current(self, pins_list: List[Dict[str, Any]]) -> float:
        """
        Estimate total current draw from pin assignments.

        Uses 5mA per output pin as default estimate for ESP32.
        Non-dict entries are skipped (they are rejected during validation).
        """
        total = 0.0
        for pin_assignment in pins_list:
            if not isinstance(pin_assignment, dict):
                continue
            function = str(pin_assignment.get("function") or "")
            protocol_bus = str(pin_assignment.get("protocol_bus") or "")
            direction = str(pin_assignment.get("direction") or "")

            if self._is_output_function(function, protocol_bus, direction):
                total += 5.0  # mA estimated draw per output pin (ESP32 default)

        return total

    # ESP32 strapping pins with boot behavior descriptions
    _STRAPPING_MSGS_ESP32 = {
        0: "Must be HIGH at boot (default pull-up). Ensure external circuit does not pull LOW during reset.",
        2: "Must be LOW at boot for download mode. Avoid external pull-ups that conflict.",
        5: "Controls SDIO timing. Default pull-up. Usually safe for general use.",
        12: "Sets flash voltage (LOW=3.3V, HIGH=1.8V). CRITICAL: Ensure LOW at boot via 10kΩ pull-down if using 3.3V flash.",
        15: "Controls UART boot log output. Default pull-up. Usually safe for general use.",
    }

    # ESP32-S2 strapping pins
    _STRAPPING_MSGS_ESP32S2 = {
        0: "Must be HIGH for normal boot (default pull-up).",
        45: "VDD_SPI voltage select. CRITICAL: If HIGH at boot, selects 1.8V which can damage 3.3V flash.",
        46: "Controls boot mode. Must be LOW for SPI boot.",
    }

    # ESP32-S3 strapping pins
    _STRAPPING_MSGS_ESP32S3 = {
        0: "Must be HIGH for normal boot (default pull-up).",
        3: "JTAG signal source select. Default pull-up.",
        45: "VDD_SPI voltage select. CRITICAL: If HIGH at boot, selects 1.8V which can damage 3.3V flash.",
        46: "Controls boot mode and ROM log output. Must be LOW for SPI boot.",
    }

    # ESP32-C3 strapping pins
    _STRAPPING_MSGS_ESP32C3 = {
        2: "Controls boot mode. Must be HIGH for normal boot.",
        8: "Controls boot mode and ROM log output.",
        9: "Controls boot mode. Must be HIGH for normal boot.",
    }

    # ESP32-C6 strapping pins
    _STRAPPING_MSGS_ESP32C6 = {
        4: "MTMS - JTAG signal.",
        5: "MTDI - JTAG signal.",
        8: "Controls boot mode.",
        9: "Controls boot mode. Must be HIGH for normal boot.",
        15: "Controls boot mode.",
    }

    # ESP32 ADC1 channels (always available)
    _ADC1_CHANNELS_ESP32 = {
        36: 0, 37: 1, 38: 2, 39: 3, 32: 4, 33: 5, 34: 6, 35: 7
    }

    # ESP32 ADC2 channels (unavailable during WiFi)
    _ADC2_CHANNELS_ESP32 = {
        4: 0, 0: 1, 2: 2, 15: 3, 13: 4, 12: 5, 14: 6, 27: 7, 25: 8, 26: 9
    }

    def _get_flash_pins(self) -> Set[int]:
        """
        Get the flash SPI pins for the current variant.

        Each ESP32 variant reserves different GPIO ranges for flash SPI.
        """
        variant_lower = self.variant.lower()
        if variant_lower == "esp32":
            return _FLASH_PINS_ESP32
        if "s3" in variant_lower:
            return _FLASH_PINS_ESP32S3
        if "s2" in variant_lower:
            return _FLASH_PINS_ESP32S2
        if "c3" in variant_lower:
            return _FLASH_PINS_ESP32C3
        if "c6" in variant_lower:
            return _FLASH_PINS_ESP32C6
        return _FLASH_PINS_ESP32

    def _get_strapping_pin_set(self) -> Set[int]:
        """Get the set of strapping pin numbers for the current variant."""
        variant_lower = self.variant.lower()
        if variant_lower == "esp32":
            return _STRAPPING_PINS_ESP32_SET
        if "s2" in variant_lower:
            return _STRAPPING_PINS_ESP32S2_SET
        if "s3" in variant_lower:
            return _STRAPPING_PINS_ESP32S3_SET
        if "c3" in variant_lower:
            return _STRAPPING_PINS_ESP32C3_SET
        if "c6" in variant_lower:
            return _STRAPPING_PINS_ESP32C6_SET
        # Default to base ESP32 for unknown variants
        return _STRAPPING_PINS_ESP32_SET

    def _get_strapping_pins(self) -> Dict[int, str]:
        """Get the strapping pins with messages for the current variant."""
        variant_lower = self.variant.lower()
        if variant_lower == "esp32":
            return self._STRAPPING_MSGS_ESP32
        if "s2" in variant_lower:
            return self._STRAPPING_MSGS_ESP32S2
        if "s3" in variant_lower:
            return self._STRAPPING_MSGS_ESP32S3
        if "c3" in variant_lower:
            return self._STRAPPING_MSGS_ESP32C3
        if "c6" in variant_lower:
            return self._STRAPPING_MSGS_ESP32C6
        # Default to base ESP32 for unknown variants
        return self._STRAPPING_MSGS_ESP32

    def _get_adc_info(self, gpio: int) -> Optional[str]:
        """Get ADC channel info for a GPIO pin (variant-aware)."""
        variant_lower = self.variant.lower()
        if "s3" in variant_lower or "s2" in variant_lower:
            # ESP32-S2/S3: GPIO1-10 = ADC1, GPIO11-20 = ADC2
            if 1 <= gpio <= 10:
                return f"ADC1_CH{gpio - 1}"
            elif 11 <= gpio <= 20:
                return f"ADC2_CH{gpio - 11}"
            return None
        elif "c3" in variant_lower:
            # ESP32-C3: GPIO0-4 = ADC1 (CH0-CH4), no separate ADC2 in practice
            if 0 <= gpio <= 4:
                return f"ADC1_CH{gpio}"
            return None
        elif "c6" in variant_lower:
            # ESP32-C6: GPIO0-6 = ADC1 (CH0-CH6)
            if 0 <= gpio <= 6:
                return f"ADC1_CH{gpio}"
            return None
        else:
            # Original ESP32
            if gpio in self._ADC1_CHANNELS_ESP32:
                return f"ADC1_CH{self._ADC1_CHANNELS_ESP32[gpio]}"
            elif gpio in self._ADC2_CHANNELS_ESP32:
                return f"ADC2_CH{self._ADC2_CHANNELS_ESP32[gpio]}"
            return None

    def _is_adc2_pin(self, gpio: int) -> bool:
        """Check if a GPIO is an ADC2 pin (variant-aware)."""
        return gpio in self._get_adc2_gpios()

    def _normalize_pins(self, pins_list: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Normalize pin dicts to ensure all string fields are non-None."""
        normalized = []
        for pin in pins_list:
            if not isinstance(pin, dict):
                continue
            p = dict(pin)
            for field in ("function", "protocol_bus", "device", "notes", "pull", "direction"):
                p[field] = str(p.get(field) or "")
            # Coerce gpio to int — reject non-numeric and fractional values
            raw_gpio = p.get("gpio", 0)
            if isinstance(raw_gpio, bool):
                raise ValueError(f"Invalid gpio value '{raw_gpio}': must be an integer, not boolean")
            if isinstance(raw_gpio, float) and raw_gpio != int(raw_gpio):
                raise ValueError(f"Invalid gpio value '{raw_gpio}': must be an integer, not float")
            try:
                p["gpio"] = int(raw_gpio if raw_gpio is not None else 0)
            except (TypeError, ValueError):
                raise ValueError(f"Invalid gpio value '{raw_gpio}': must be an integer")
            # Coerce speed_hz to int — reject non-numeric values
            raw_speed = p.get("speed_hz", 0)
            try:
                p["speed_hz"] = int(raw_speed if raw_speed is not None else 0)
            except (TypeError, ValueError):
                raise ValueError(f"Invalid speed_hz value '{raw_speed}': must be an integer")
            normalized.append(p)
        return normalized

    @staticmethod
    def _sanitize_comment(text: str) -> str:
        """Sanitize a string for safe inclusion in C/C++ comments.

        Removes characters that could break block or line comments.
        """
        # Remove block comment delimiters and newlines
        text = text.replace("*/", "").replace("/*", "")
        text = text.replace("\n", " ").replace("\r", " ")
        return text

    def _sanitize_var_name(self, name: str, gpio: int, function: str, uppercase: bool = False) -> str:
        """
        Convert a device name to a valid C identifier.

        Args:
            name: Device name (may be empty).
            gpio: GPIO number (used if name is empty).
            function: Function name (used if name is empty).
            uppercase: If True, return UPPER_CASE for constants.

        Returns:
            Valid C identifier.
        """
        if not name or not name.strip():
            name = f"{function}_{gpio}"

        # Replace non-alphanumeric with underscore
        result = ""
        for char in name:
            if char.isalnum():
                result += char
            elif char in (" ", "-", "_"):
                result += "_"

        # Remove consecutive underscores
        while "__" in result:
            result = result.replace("__", "_")

        # Remove leading/trailing underscores
        result = result.strip("_")

        # Ensure it doesn't start with a digit
        if result and result[0].isdigit():
            result = "dev_" + result

        if not result:
            result = f"gpio_{gpio}"

        if uppercase:
            return result.upper()
        return result.lower()

    def _categorize_pins(
        self, pins_list: List[Dict[str, Any]]
    ) -> Dict[str, List[Dict[str, Any]]]:
        """Categorize pins by protocol/function."""
        categories = {
            "i2c": [],
            "spi": [],
            "uart": [],
            "pwm": [],
            "onewire": [],
            "adc": [],
            "gpio_output": [],
            "gpio_input": [],
        }

        for pin in pins_list:
            protocol = (pin.get("protocol_bus") or "").lower()
            function = (pin.get("function") or "").upper()

            if protocol == "i2c":
                categories["i2c"].append(pin)
            elif protocol == "spi":
                categories["spi"].append(pin)
            elif protocol == "uart":
                categories["uart"].append(pin)
            elif protocol == "pwm" or "PWM" in function:
                categories["pwm"].append(pin)
            elif protocol in ("1wire", "onewire") or "1-WIRE" in function:
                categories["onewire"].append(pin)
            elif protocol == "adc" or "ADC" in function or "ANALOG" in function:
                categories["adc"].append(pin)
            elif self._is_output_function(
                pin.get("function", ""), pin.get("protocol_bus", ""),
                pin.get("direction", "")
            ):
                categories["gpio_output"].append(pin)
            else:
                # Default: treat as input (includes INPUT, unknown functions, empty protocol)
                categories["gpio_input"].append(pin)

        return categories

    def _build_wiring_note(
        self,
        gpio: int,
        function: str,
        protocol: str,
        device: str,
        pull: str,
        notes: str
    ) -> str:
        """Build a wiring note for a single ESP32 pin."""
        func_upper = function.upper()
        device_str = device if device else "device"

        # Determine context based on protocol and function
        if protocol == "i2c":
            if "sda" in function.lower():
                context = "SDA"
            elif "scl" in function.lower():
                context = "SCL"
            else:
                context = function
        elif protocol == "spi":
            context = function  # MOSI, MISO, SCLK, CS
        elif protocol == "uart":
            context = f"{function} (cross TX↔RX with device)"
        elif protocol == "adc" or "ADC" in func_upper:
            adc_info = self._get_adc_info(gpio)
            if adc_info:
                context = f"analog input ({adc_info})"
            else:
                context = "analog input"
        elif func_upper == "OUTPUT":
            context = "anode via 330Ω resistor" if "led" in device.lower() else "output"
        elif func_upper == "INPUT":
            if pull in ("internal_up", "up"):
                context = "input with internal pull-up"
            elif pull in ("internal_down", "down"):
                context = "input with internal pull-down"
            elif pull == "external_up":
                context = "input with external pull-up"
            elif pull == "external_down":
                context = "input with external pull-down"
            else:
                context = "input"
        else:
            context = function

        note = f"Connect {device_str} {context} to GPIO{gpio}"

        if notes:
            note += f". {notes}"

        # Add strapping pin warning if applicable
        strapping_pins = self._get_strapping_pins()
        if gpio in strapping_pins:
            note += f". ⚠ STRAPPING PIN: {strapping_pins[gpio]}"

        return note

    def generate_config(self, assignment: Dict[str, Any]) -> GenerationResult:
        """
        Generate platform configuration for an ESP32 pin assignment.

        Produces sdkconfig hints (as comments), wiring notes, and warnings
        for the given assignment. ESP32 configures everything in code,
        so config_lines are informational.

        Args:
            assignment: Pin assignment dict with keys: platform, variant,
                module, wifi_enabled, pins (list of pin assignments).

        Returns:
            GenerationResult with config_lines, wiring_notes, warnings,
            and empty init_code (code is produced by generate_code()).
        """
        pins_list = self._normalize_pins(assignment.get("pins", []))
        variant = assignment.get("variant", self.variant)
        module = assignment.get("module", self.module)
        wifi_enabled = assignment.get("wifi_enabled", False)

        # Normalize module
        if module:
            module = module.upper()
            if module in ("WROOM32", "WROOM-32"):
                module = "WROOM"
            elif module in ("WROVER32", "WROVER-32"):
                module = "WROVER"

        config_lines = []  # type: List[str]
        wiring_notes = []  # type: List[str]
        warnings = []  # type: List[str]

        # Categorize pins
        categories = self._categorize_pins(pins_list)

        # Build config_lines (informational comments for ESP32)
        config_lines.append(f"# ESP32 Configuration Notes ({variant})")
        config_lines.append("")

        if wifi_enabled:
            config_lines.append("# sdkconfig: CONFIG_ESP_WIFI_ENABLED=y")
            config_lines.append("# Note: ADC2 pins unavailable during WiFi operation")

        if categories["i2c"]:
            config_lines.append("# I2C configured in code (no separate config needed)")

        if categories["spi"]:
            config_lines.append("# SPI configured in code (no separate config needed)")

        if categories["uart"]:
            config_lines.append("# UART configured in code (no separate config needed)")

        if categories["pwm"]:
            config_lines.append("# LEDC PWM configured in code")

        if categories["adc"]:
            config_lines.append("# ADC configured in code")

        if categories["onewire"]:
            config_lines.append("# 1-Wire configured in code")

        config_lines.append("")

        # Add pin documentation
        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            function = pin.get("function", "")
            device = pin.get("device", "")
            config_lines.append(f"# GPIO{gpio}: {device} ({function})")

        # Build wiring notes for each pin
        strapping_pins = self._get_strapping_pins()
        strapping_gpios_used = []

        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            function = pin.get("function", "")
            protocol = pin.get("protocol_bus", "").lower()
            device = pin.get("device", "")
            pull = pin.get("pull", "none")
            notes = pin.get("notes", "")

            wiring_note = self._build_wiring_note(
                gpio, function, protocol, device, pull, notes
            )
            wiring_notes.append(wiring_note)

            # Track strapping pins for warnings
            if gpio in strapping_pins:
                strapping_gpios_used.append(gpio)

        # Add protocol-level wiring notes
        if categories["i2c"]:
            all_have_external_pullup = all(
                (p.get("pull") or "") == "external_up" for p in categories["i2c"]
            )
            if not all_have_external_pullup:
                wiring_notes.append(
                    "Add 4.7kΩ pull-up resistors on SDA and SCL to 3.3V"
                )

        if categories["onewire"]:
            wiring_notes.append(
                "Add 4.7kΩ pull-up resistor on 1-Wire data line to 3.3V"
            )

        # Add module-specific note
        if module == "WROVER":
            wiring_notes.append(
                "Note: GPIO16 and GPIO17 are used by PSRAM on WROVER modules and cannot be used."
            )

        # Add power note
        wiring_notes.append(
            "Power: ESP32 operates at 3.3V logic. Do not apply 5V to GPIO pins."
        )

        # Generate warnings
        # Strapping pin warnings - variant-aware
        variant_lower = variant.lower() if variant else self.variant.lower()
        # Determine which GPIO has flash voltage risk for this variant
        flash_voltage_gpio = None
        if variant_lower == "esp32":
            flash_voltage_gpio = 12
        elif "s2" in variant_lower or "s3" in variant_lower:
            flash_voltage_gpio = 45

        for gpio in strapping_gpios_used:
            if gpio == flash_voltage_gpio:
                # Elevated severity for flash voltage pins
                warnings.append(
                    f"GPIO{gpio} is a strapping pin that sets flash voltage. "
                    f"CRITICAL: If pulled HIGH at boot, it selects 1.8V flash which can damage "
                    f"3.3V flash chips. Ensure GPIO{gpio} is LOW at boot via 10kΩ pull-down resistor."
                )
            else:
                warnings.append(
                    f"GPIO{gpio} is a strapping pin: {strapping_pins[gpio]}"
                )

        # ADC2 + WiFi warnings
        if wifi_enabled:
            for pin in pins_list:
                gpio = pin.get("gpio", 0)
                function = pin.get("function", "").lower()
                protocol = pin.get("protocol_bus", "").lower()

                is_adc_use = "adc" in function or "analog" in function or protocol == "adc"
                if is_adc_use and self._is_adc2_pin(gpio):
                    warnings.append(
                        f"GPIO{gpio} uses ADC2 which is unavailable during WiFi. "
                        f"Consider ADC1 pins instead."
                    )

        # WROVER module GPIO16/17 warning
        if module == "WROVER":
            for pin in pins_list:
                gpio = pin.get("gpio", 0)
                if gpio in (16, 17):
                    warnings.append(
                        f"GPIO{gpio} is unavailable on WROVER modules (PSRAM conflict)"
                    )

        return GenerationResult(
            config_lines=config_lines,
            init_code="",
            wiring_notes=wiring_notes,
            warnings=warnings,
            alternatives=[]
        )

    def generate_code(self, assignment: Dict[str, Any], framework: str) -> str:
        """
        Generate initialization code for an ESP32 pin assignment.

        Produces complete, syntactically valid source code for the specified
        framework (Arduino C++ or ESP-IDF C).

        Args:
            assignment: Pin assignment dict (same format as validate() input).
            framework: Target framework - "arduino" or "espidf".

        Returns:
            Complete source file as a string.

        Raises:
            ValueError: If the framework is not supported.
        """
        framework = framework.lower()
        if framework not in ("arduino", "espidf"):
            raise ValueError(
                f"Unsupported ESP32 framework: {framework}. "
                f"Use 'arduino' or 'espidf'."
            )

        if framework == "arduino":
            return self._generate_arduino_code(assignment)
        else:
            return self._generate_espidf_code(assignment)

    def _build_header_comment(
        self, assignment: Dict[str, Any], framework: str
    ) -> str:
        """Build the header comment for generated code."""
        pins_list = assignment.get("pins", [])
        variant = assignment.get("variant", self.variant)

        lines = ["/**"]
        lines.append(f" * GPIO initialization for ESP32 ({variant})")
        lines.append(f" * Framework: {framework}")
        lines.append(" *")
        lines.append(" * Pin assignments:")

        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            protocol = self._sanitize_comment(pin.get("protocol_bus", ""))
            function = self._sanitize_comment(pin.get("function", ""))
            device = self._sanitize_comment(pin.get("device", ""))
            lines.append(f" *   GPIO{gpio} - {protocol} {function} ({device})")

        lines.append(" *")
        lines.append(f" * Board: ESP32 {variant}")
        lines.append(" */")
        return "\n".join(lines)

    def _generate_arduino_code(self, assignment: Dict[str, Any]) -> str:
        """Generate Arduino framework code for ESP32."""
        pins_list = self._normalize_pins(assignment.get("pins", []))
        categories = self._categorize_pins(pins_list)

        lines = []

        # Header comment
        lines.append(self._build_header_comment(assignment, "Arduino"))
        lines.append("")

        # Includes
        includes = self._build_arduino_includes(categories)
        lines.extend(includes)
        lines.append("")

        # Build canonical symbol map once, reuse everywhere
        symbol_map = self._build_pin_symbol_map(pins_list)

        # Pin definitions
        pin_defs = self._build_arduino_pin_definitions(pins_list, symbol_map)
        if pin_defs:
            lines.append("// Pin definitions")
            lines.extend(pin_defs)
            lines.append("")

        # I2C address constants
        i2c_addrs = self._build_i2c_address_constants(categories)
        if i2c_addrs:
            lines.extend(i2c_addrs)
            lines.append("")

        # 1-Wire objects
        onewire_objs = self._build_arduino_onewire_objects(categories, symbol_map)
        if onewire_objs:
            lines.extend(onewire_objs)
            lines.append("")

        # PWM channel tracking
        pwm_channels = self._build_arduino_pwm_channels(categories, symbol_map)
        if pwm_channels:
            lines.extend(pwm_channels)
            lines.append("")

        # Setup function
        lines.append("void setup() {")
        lines.append("    Serial.begin(115200);")
        lines.append("")

        # I2C initialization
        if categories["i2c"]:
            i2c_init = self._build_arduino_i2c_init(categories, symbol_map)
            lines.extend(i2c_init)
            lines.append("")

        # SPI initialization
        if categories["spi"]:
            spi_init = self._build_arduino_spi_init(categories, symbol_map)
            lines.extend(spi_init)
            lines.append("")

        # UART initialization
        if categories["uart"]:
            uart_init = self._build_arduino_uart_init(categories, symbol_map)
            lines.extend(uart_init)
            lines.append("")

        # GPIO outputs
        if categories["gpio_output"]:
            lines.append("    // GPIO outputs")
            for pin in categories["gpio_output"]:
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}")
                lines.append(f"    pinMode({const_name}_PIN, OUTPUT);")
                lines.append(f"    digitalWrite({const_name}_PIN, LOW);")
            lines.append("")

        # GPIO inputs
        if categories["gpio_input"]:
            input_only_pins = self._get_input_only_pins()
            lines.append("    // GPIO inputs")
            for pin in categories["gpio_input"]:
                gpio = pin.get("gpio", 0)
                pull = pin.get("pull", "none")
                const_name = symbol_map.get(gpio, f"GPIO{gpio}")
                # Input-only pins have no internal pulls — always use plain INPUT
                if gpio in input_only_pins:
                    lines.append(f"    pinMode({const_name}_PIN, INPUT);  // input-only, use external pull")
                elif pull in ("internal_up", "up"):
                    lines.append(f"    pinMode({const_name}_PIN, INPUT_PULLUP);")
                elif pull in ("internal_down", "down"):
                    lines.append(f"    pinMode({const_name}_PIN, INPUT_PULLDOWN);")
                else:
                    lines.append(f"    pinMode({const_name}_PIN, INPUT);")
            lines.append("")

        # PWM setup
        if categories["pwm"]:
            pwm_setup = self._build_arduino_pwm_setup(categories, symbol_map)
            lines.extend(pwm_setup)
            lines.append("")

        # ADC setup
        if categories["adc"]:
            lines.append("    // ADC setup")
            lines.append("    analogReadResolution(12);  // 12-bit (0-4095)")
            lines.append("    // analogSetAttenuation(ADC_11db);  // Full range 0-3.3V")
            lines.append("")

        # 1-Wire setup
        if categories["onewire"]:
            lines.append("    // 1-Wire setup")
            for i, _pin in enumerate(categories["onewire"]):
                suffix = f"_{i}" if len(categories["onewire"]) > 1 else ""
                lines.append(f"    sensors{suffix}.begin();")
            lines.append("")

        lines.append('    Serial.println("GPIO initialized");')
        lines.append("}")
        lines.append("")

        # Loop function
        lines.append("void loop() {")
        loop_body = self._build_arduino_loop(categories, symbol_map)
        lines.extend(loop_body)
        lines.append("}")
        lines.append("")

        return "\n".join(lines)

    def _build_arduino_includes(
        self, categories: Dict[str, List[Dict[str, Any]]]
    ) -> List[str]:
        """Build Arduino include statements."""
        lines = []

        if categories["i2c"]:
            lines.append("#include <Wire.h>")

        if categories["spi"]:
            lines.append("#include <SPI.h>")

        if categories["onewire"]:
            lines.append("#include <OneWire.h>")
            lines.append("#include <DallasTemperature.h>")

        if not lines:
            lines.append("// No additional includes needed")

        return lines

    def _build_pin_symbol_map(
        self, pins_list: List[Dict[str, Any]]
    ) -> Dict[int, str]:
        """Build canonical GPIO -> symbol name map, used by all code generators."""
        symbol_map = {}  # type: Dict[int, str]
        seen_names = set()  # type: Set[str]

        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            device = str(pin.get("device") or "")
            function = str(pin.get("function") or "")
            protocol = str(pin.get("protocol_bus") or "").lower()

            # For protocol pins (I2C, SPI, UART), include function in name
            if protocol in ("i2c", "spi", "uart"):
                name_base = f"{device}_{function}" if device else function
            else:
                name_base = device if device else function

            const_name = self._sanitize_var_name(name_base, gpio, function, uppercase=True)

            # Handle duplicate names by appending GPIO number
            if const_name in seen_names:
                const_name = f"{const_name}_{gpio}"
            seen_names.add(const_name)

            symbol_map[gpio] = const_name

        return symbol_map

    def _build_arduino_pin_definitions(
        self, pins_list: List[Dict[str, Any]], symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino pin constant definitions using canonical symbol map."""
        lines = []

        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            device = self._sanitize_comment(str(pin.get("device") or ""))
            notes = self._sanitize_comment(str(pin.get("notes") or ""))
            const_name = symbol_map.get(gpio, f"GPIO{gpio}")

            comment = f"  // {device}" if device else ""
            if notes:
                comment = f"  // {notes}" if not comment else f"{comment} - {notes}"

            lines.append(f"const int {const_name}_PIN = {gpio};{comment}")

        return lines

    def _build_i2c_address_constants(
        self, categories: Dict[str, List[Dict[str, Any]]]
    ) -> List[str]:
        """Build I2C address constant placeholders."""
        lines = []

        if categories["i2c"]:
            lines.append("// I2C address constants")
            devices_seen = set()
            for pin in categories["i2c"]:
                device = pin.get("device", "")
                if device and device not in devices_seen:
                    devices_seen.add(device)
                    const_name = self._sanitize_var_name(device, 0, "i2c", uppercase=True)
                    lines.append(f"const uint8_t {const_name}_ADDR = 0x00;  // Replace with actual I2C address for {device}")

        return lines

    def _build_arduino_onewire_objects(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino 1-Wire object declarations."""
        lines = []

        if categories["onewire"]:
            lines.append("// 1-Wire objects")
            for i, pin in enumerate(categories["onewire"]):
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"ONEWIRE_{gpio}")
                var_name = const_name.lower()
                suffix = f"_{i}" if len(categories["onewire"]) > 1 else ""
                lines.append(f"OneWire {var_name}OneWire({const_name}_PIN);")
                lines.append(f"DallasTemperature sensors{suffix}(&{var_name}OneWire);")

        return lines

    def _build_arduino_pwm_channels(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino PWM channel constants using canonical symbol map."""
        lines = []

        if categories["pwm"]:
            variant_lower = self.variant.lower()
            max_channels = 6 if ("c3" in variant_lower or "c6" in variant_lower) else 16
            lines.append(f"// LEDC PWM channels (max {max_channels})")
            for i, pin in enumerate(categories["pwm"][:max_channels]):
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"PWM{gpio}")
                lines.append(f"const int {const_name}_CHANNEL = {i};")

        return lines

    def _build_arduino_i2c_init(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino I2C initialization code."""
        lines = []

        if categories["i2c"]:
            # Find SDA and SCL pins using canonical symbol_map
            sda_pin = None
            scl_pin = None
            for pin in categories["i2c"]:
                func = pin.get("function", "").lower()
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
                if "sda" in func or "data" in func:
                    sda_pin = const_name
                elif "scl" in func or "clock" in func:
                    scl_pin = const_name

            lines.append("    // I2C initialization")
            if sda_pin and scl_pin:
                lines.append(f"    Wire.begin({sda_pin}, {scl_pin});")
            else:
                lines.append("    Wire.begin();  // Using default I2C pins")
            # Set I2C clock from speed_hz if specified
            i2c_speed = None
            for pin in categories["i2c"]:
                speed = pin.get("speed_hz", 0)
                if speed and speed > 0:
                    i2c_speed = speed
                    break
            if i2c_speed:
                lines.append(f"    Wire.setClock({i2c_speed});  // Hz")

        return lines

    def _build_arduino_spi_init(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino SPI initialization code."""
        lines = []

        if categories["spi"]:
            # Find SPI pins using canonical symbol_map
            sclk = miso = mosi = cs = None
            for pin in categories["spi"]:
                func = pin.get("function", "").lower()
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
                if "sclk" in func or "clk" in func:
                    sclk = const_name
                elif "miso" in func:
                    miso = const_name
                elif "mosi" in func:
                    mosi = const_name
                elif "cs" in func or "ss" in func:
                    cs = const_name

            lines.append("    // SPI initialization")
            if sclk and mosi:
                miso_arg = miso if miso else "-1"
                if cs:
                    lines.append(f"    SPI.begin({sclk}, {miso_arg}, {mosi}, {cs});")
                else:
                    lines.append(f"    SPI.begin({sclk}, {miso_arg}, {mosi});")
            else:
                lines.append("    SPI.begin();  // Using default SPI pins")

        return lines

    def _build_arduino_uart_init(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino UART initialization code."""
        lines = []

        if categories["uart"]:
            # Find TX and RX pins using canonical symbol_map
            tx_pin = rx_pin = None
            for pin in categories["uart"]:
                func = pin.get("function", "").lower()
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
                if "tx" in func:
                    tx_pin = const_name
                elif "rx" in func:
                    rx_pin = const_name

            lines.append("    // UART initialization (Serial1)")
            # Use speed_hz from UART pins if specified, default 9600
            uart_baud = 9600
            for pin in categories["uart"]:
                speed = pin.get("speed_hz", 0)
                if speed and speed > 0:
                    uart_baud = speed
                    break
            if rx_pin and tx_pin:
                lines.append(f"    Serial1.begin({uart_baud}, SERIAL_8N1, {rx_pin}, {tx_pin});")
            else:
                lines.append(f"    Serial1.begin({uart_baud});  // Using default UART pins")

        return lines

    def _build_arduino_pwm_setup(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino PWM setup code using canonical symbol map."""
        lines = []

        if categories["pwm"]:
            variant_lower = self.variant.lower()
            max_channels = 6 if ("c3" in variant_lower or "c6" in variant_lower) else 16
            lines.append(f"    // PWM setup (LEDC, max {max_channels} channels)")
            lines.append("    // Note: ESP32 Arduino core >=3.0 uses ledcAttach(pin, freq, resolution)")
            for pin in categories["pwm"][:max_channels]:
                gpio = pin.get("gpio", 0)
                speed = pin.get("speed_hz", 0)
                freq = speed if speed and speed > 0 else 1000
                const_name = symbol_map.get(gpio, f"PWM{gpio}")
                lines.append(f"    ledcAttach({const_name}_PIN, {freq}, 8);  // {freq}Hz, 8-bit resolution")

        return lines

    def _build_arduino_loop(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build Arduino loop function body."""
        lines = []
        lines.append("    // Example usage")

        if categories["gpio_input"] and categories["gpio_output"]:
            in_pin = categories["gpio_input"][0]
            out_pin = categories["gpio_output"][0]
            in_const = symbol_map.get(in_pin.get("gpio", 0), "IN")
            out_const = symbol_map.get(out_pin.get("gpio", 0), "OUT")
            lines.append(f"    if (digitalRead({in_const}_PIN) == LOW) {{")
            lines.append(f"        digitalWrite({out_const}_PIN, !digitalRead({out_const}_PIN));")
            lines.append("        delay(200);  // debounce")
            lines.append("    }")
        elif categories["gpio_output"]:
            out_pin = categories["gpio_output"][0]
            out_const = symbol_map.get(out_pin.get("gpio", 0), "OUT")
            lines.append(f"    // digitalWrite({out_const}_PIN, HIGH);")
            lines.append("    // delay(1000);")
            lines.append(f"    // digitalWrite({out_const}_PIN, LOW);")
            lines.append("    // delay(1000);")
        else:
            lines.append("    // Add your main loop code here")
            lines.append("    delay(100);")

        return lines

    def _generate_espidf_code(self, assignment: Dict[str, Any]) -> str:
        """Generate ESP-IDF framework code for ESP32."""
        pins_list = self._normalize_pins(assignment.get("pins", []))
        categories = self._categorize_pins(pins_list)

        lines = []

        # Header comment
        lines.append(self._build_header_comment(assignment, "ESP-IDF"))
        lines.append("")

        # Includes
        includes = self._build_espidf_includes(categories)
        lines.extend(includes)
        lines.append("")

        # TAG for logging
        lines.append('static const char *TAG = "gpio_init";')
        lines.append("")

        # Build canonical symbol map once
        symbol_map = self._build_pin_symbol_map(pins_list)

        # Pin definitions
        pin_defs = self._build_espidf_pin_definitions(pins_list, symbol_map)
        if pin_defs:
            lines.append("// Pin definitions")
            lines.extend(pin_defs)
            lines.append("")

        # Protocol constants
        proto_consts = self._build_espidf_protocol_constants(categories)
        if proto_consts:
            lines.extend(proto_consts)
            lines.append("")

        # I2C init function
        if categories["i2c"]:
            i2c_func = self._build_espidf_i2c_init_func(categories, symbol_map)
            lines.extend(i2c_func)
            lines.append("")

        # SPI init function
        if categories["spi"]:
            spi_func = self._build_espidf_spi_init_func(categories, symbol_map)
            lines.extend(spi_func)
            lines.append("")

        # UART init function
        if categories["uart"]:
            uart_func = self._build_espidf_uart_init_func(categories, symbol_map)
            lines.extend(uart_func)
            lines.append("")

        # app_main function
        lines.append("void app_main(void) {")
        lines.append('    ESP_LOGI(TAG, "Initializing GPIO");')
        lines.append("")

        # Call init functions
        if categories["i2c"]:
            lines.append("    // I2C")
            lines.append("    ESP_ERROR_CHECK(i2c_master_init());")
            lines.append("")

        if categories["spi"]:
            lines.append("    // SPI")
            lines.append("    ESP_ERROR_CHECK(spi_master_init());")
            lines.append("")

        if categories["uart"]:
            lines.append("    // UART")
            lines.append("    ESP_ERROR_CHECK(uart_init());")
            lines.append("")

        # 1-Wire (GPIO setup only — protocol handled by library)
        if categories["onewire"]:
            for pin in categories["onewire"]:
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}")
                lines.append(f"    // 1-Wire on GPIO{gpio}")
                lines.append(f"    gpio_reset_pin({const_name}_PIN);")
                lines.append(f"    gpio_set_direction({const_name}_PIN, GPIO_MODE_INPUT_OUTPUT_OD);")
                lines.append(f"    gpio_set_pull_mode({const_name}_PIN, GPIO_PULLUP_ONLY);")
            lines.append("")

        # GPIO outputs
        if categories["gpio_output"]:
            gpio_out = self._build_espidf_gpio_output_config(categories, symbol_map)
            lines.extend(gpio_out)
            lines.append("")

        # GPIO inputs - group by pull configuration
        if categories["gpio_input"]:
            gpio_in = self._build_espidf_gpio_input_config(categories, symbol_map)
            lines.extend(gpio_in)
            lines.append("")

        # PWM (LEDC)
        if categories["pwm"]:
            pwm_config = self._build_espidf_pwm_config(categories, symbol_map)
            lines.extend(pwm_config)
            lines.append("")

        # ADC
        if categories["adc"]:
            adc_config = self._build_espidf_adc_config(categories)
            lines.extend(adc_config)
            lines.append("")

        lines.append('    ESP_LOGI(TAG, "GPIO initialized");')
        lines.append("}")
        lines.append("")

        return "\n".join(lines)

    def _build_espidf_includes(
        self, categories: Dict[str, List[Dict[str, Any]]]
    ) -> List[str]:
        """Build ESP-IDF include statements."""
        lines = []

        lines.append("#include <stdio.h>")
        lines.append('#include "driver/gpio.h"')

        if categories["i2c"]:
            lines.append('#include "driver/i2c.h"')

        if categories["spi"]:
            lines.append('#include "driver/spi_master.h"')

        if categories["uart"]:
            lines.append('#include "driver/uart.h"')

        if categories["pwm"]:
            lines.append('#include "driver/ledc.h"')

        if categories["adc"]:
            lines.append('#include "driver/adc.h"')

        lines.append('#include "esp_log.h"')

        return lines

    def _build_espidf_pin_definitions(
        self, pins_list: List[Dict[str, Any]], symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF pin macro definitions using canonical symbol map."""
        lines = []

        for pin in pins_list:
            gpio = pin.get("gpio", 0)
            device = self._sanitize_comment(str(pin.get("device") or ""))
            notes = self._sanitize_comment(str(pin.get("notes") or ""))
            const_name = symbol_map.get(gpio, f"GPIO{gpio}")

            comment = f"  // {device}" if device else ""
            if notes:
                comment = f"  // {notes}" if not comment else f"{comment} - {notes}"

            lines.append(f"#define {const_name}_PIN GPIO_NUM_{gpio}{comment}")

        return lines

    def _build_espidf_protocol_constants(
        self, categories: Dict[str, List[Dict[str, Any]]]
    ) -> List[str]:
        """Build ESP-IDF protocol constants."""
        lines = []

        if categories["i2c"]:
            # Use speed_hz from I2C pins if specified, default 100kHz
            i2c_freq = 100000
            for pin in categories["i2c"]:
                speed = pin.get("speed_hz", 0)
                if speed and speed > 0:
                    i2c_freq = speed
                    break
            lines.append("// I2C constants")
            lines.append("#define I2C_MASTER_NUM    I2C_NUM_0")
            lines.append(f"#define I2C_MASTER_FREQ   {i2c_freq}")
            # Add device address placeholders
            devices_seen = set()
            for pin in categories["i2c"]:
                device = pin.get("device", "")
                if device and device not in devices_seen:
                    devices_seen.add(device)
                    const_name = self._sanitize_var_name(device, 0, "i2c", uppercase=True)
                    lines.append(f"#define {const_name}_ADDR 0x00  // Replace with actual I2C address for {device}")

        if categories["uart"]:
            lines.append("// UART constants")
            lines.append("#define UART_NUM    UART_NUM_1")
            lines.append("#define UART_BUF_SIZE 256")

        return lines

    def _build_espidf_i2c_init_func(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF I2C initialization function."""
        lines = []

        # Find SDA and SCL using canonical symbol_map
        sda_def = scl_def = None
        for pin in categories["i2c"]:
            func = pin.get("function", "").lower()
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
            if "sda" in func or "data" in func:
                sda_def = const_name
            elif "scl" in func or "clock" in func:
                scl_def = const_name

        lines.append("// I2C initialization function")
        lines.append("static esp_err_t i2c_master_init(void) {")
        lines.append("    i2c_config_t conf = {")
        lines.append("        .mode = I2C_MODE_MASTER,")
        # Use variant-aware I2C defaults from peripheral groups
        i2c_group = self._groups.get("I2C")
        if sda_def:
            lines.append(f"        .sda_io_num = {sda_def},")
        elif i2c_group:
            lines.append(f"        .sda_io_num = GPIO_NUM_{i2c_group.pins.get('SDA', 21)},  // {self.variant} default")
        else:
            lines.append("        .sda_io_num = GPIO_NUM_21,  // default")
        if scl_def:
            lines.append(f"        .scl_io_num = {scl_def},")
        elif i2c_group:
            lines.append(f"        .scl_io_num = GPIO_NUM_{i2c_group.pins.get('SCL', 22)},  // {self.variant} default")
        else:
            lines.append("        .scl_io_num = GPIO_NUM_22,  // default")
        lines.append("        .sda_pullup_en = GPIO_PULLUP_ENABLE,")
        lines.append("        .scl_pullup_en = GPIO_PULLUP_ENABLE,")
        lines.append("        .master.clk_speed = I2C_MASTER_FREQ,")
        lines.append("    };")
        lines.append("    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);")
        lines.append("    if (err != ESP_OK) return err;")
        lines.append("    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);")
        lines.append("}")

        return lines

    def _build_espidf_spi_init_func(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF SPI initialization function."""
        lines = []

        # Find SPI pins using canonical symbol_map
        mosi = miso = sclk = None
        for pin in categories["spi"]:
            func = pin.get("function", "").lower()
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
            if "mosi" in func:
                mosi = const_name
            elif "miso" in func:
                miso = const_name
            elif "sclk" in func or "clk" in func:
                sclk = const_name

        lines.append("// SPI initialization function")
        lines.append("static esp_err_t spi_master_init(void) {")
        lines.append("    spi_bus_config_t bus_cfg = {")
        lines.append(f"        .mosi_io_num = {mosi if mosi else '-1'},")
        lines.append(f"        .miso_io_num = {miso if miso else '-1'},")
        lines.append(f"        .sclk_io_num = {sclk if sclk else '-1'},")
        lines.append("        .quadwp_io_num = -1,")
        lines.append("        .quadhd_io_num = -1,")
        lines.append("        .max_transfer_sz = 4096,  // bytes, SPI default max transfer size")
        lines.append("    };")
        lines.append("    return spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);")
        lines.append("}")

        return lines

    def _build_espidf_uart_init_func(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF UART initialization function."""
        lines = []

        # Find TX and RX pins using canonical symbol_map
        tx_def = rx_def = None
        for pin in categories["uart"]:
            func = pin.get("function", "").lower()
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"GPIO{gpio}") + "_PIN"
            if "tx" in func:
                tx_def = const_name
            elif "rx" in func:
                rx_def = const_name

        lines.append("// UART initialization function")
        lines.append("static esp_err_t uart_init(void) {")
        # Use speed_hz from UART pins if specified, default 9600
        uart_baud = 9600
        for pin in categories["uart"]:
            speed = pin.get("speed_hz", 0)
            if speed and speed > 0:
                uart_baud = speed
                break
        lines.append("    uart_config_t uart_config = {")
        lines.append(f"        .baud_rate = {uart_baud},")
        lines.append("        .data_bits = UART_DATA_8_BITS,")
        lines.append("        .parity = UART_PARITY_DISABLE,")
        lines.append("        .stop_bits = UART_STOP_BITS_1,")
        lines.append("        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,")
        lines.append("    };")
        lines.append("    esp_err_t err = uart_param_config(UART_NUM, &uart_config);")
        lines.append("    if (err != ESP_OK) return err;")
        lines.append("    err = uart_driver_install(UART_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);")
        lines.append("    if (err != ESP_OK) return err;")
        tx_pin = tx_def if tx_def else "UART_PIN_NO_CHANGE"
        rx_pin = rx_def if rx_def else "UART_PIN_NO_CHANGE"
        lines.append(f"    return uart_set_pin(UART_NUM, {tx_pin}, {rx_pin}, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);")
        lines.append("}")

        return lines

    def _build_espidf_gpio_output_config(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF GPIO output configuration."""
        lines = []

        if not categories["gpio_output"]:
            return lines

        lines.append("    // GPIO outputs")

        # Build pin bit mask for all outputs
        pin_names = []
        for pin in categories["gpio_output"]:
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"GPIO{gpio}")
            pin_names.append(f"{const_name}_PIN")

        if len(pin_names) == 1:
            mask = f"(1ULL << {pin_names[0]})"
        else:
            mask = " | ".join(f"(1ULL << {p})" for p in pin_names)

        lines.append("    gpio_config_t out_conf = {")
        lines.append(f"        .pin_bit_mask = {mask},")
        lines.append("        .mode = GPIO_MODE_OUTPUT,")
        lines.append("        .pull_up_en = GPIO_PULLUP_DISABLE,")
        lines.append("        .pull_down_en = GPIO_PULLDOWN_DISABLE,")
        lines.append("        .intr_type = GPIO_INTR_DISABLE,")
        lines.append("    };")
        lines.append("    gpio_config(&out_conf);")

        # Set initial levels
        for pin in categories["gpio_output"]:
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"GPIO{gpio}")
            lines.append(f"    gpio_set_level({const_name}_PIN, 0);")

        return lines

    def _build_espidf_gpio_input_config(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF GPIO input configuration."""
        lines = []

        if not categories["gpio_input"]:
            return lines

        lines.append("    // GPIO inputs")

        # Group by pull configuration (input-only pins forced to nopull)
        input_only_pins = self._get_input_only_pins()
        pull_groups = {}  # type: Dict[str, List[Dict[str, Any]]]
        for pin in categories["gpio_input"]:
            gpio = pin.get("gpio", 0)
            pull = pin.get("pull", "none")
            if gpio in input_only_pins:
                key = "nopull"  # Input-only pins have no internal pulls
            elif pull in ("internal_up", "up"):
                key = "pullup"
            elif pull in ("internal_down", "down"):
                key = "pulldown"
            else:
                key = "nopull"

            if key not in pull_groups:
                pull_groups[key] = []
            pull_groups[key].append(pin)

        # Generate config struct for each group
        for pull_type, pins in pull_groups.items():
            pin_names = []
            for pin in pins:
                gpio = pin.get("gpio", 0)
                const_name = symbol_map.get(gpio, f"GPIO{gpio}")
                pin_names.append(f"{const_name}_PIN")

            if len(pin_names) == 1:
                mask = f"(1ULL << {pin_names[0]})"
            else:
                mask = " | ".join(f"(1ULL << {p})" for p in pin_names)

            if pull_type == "pullup":
                pullup = "GPIO_PULLUP_ENABLE"
                pulldown = "GPIO_PULLDOWN_DISABLE"
            elif pull_type == "pulldown":
                pullup = "GPIO_PULLUP_DISABLE"
                pulldown = "GPIO_PULLDOWN_ENABLE"
            else:
                pullup = "GPIO_PULLUP_DISABLE"
                pulldown = "GPIO_PULLDOWN_DISABLE"

            struct_name = f"in_conf_{pull_type}"
            lines.append(f"    gpio_config_t {struct_name} = {{")
            lines.append(f"        .pin_bit_mask = {mask},")
            lines.append("        .mode = GPIO_MODE_INPUT,")
            lines.append(f"        .pull_up_en = {pullup},")
            lines.append(f"        .pull_down_en = {pulldown},")
            lines.append("        .intr_type = GPIO_INTR_DISABLE,")
            lines.append("    };")
            lines.append(f"    gpio_config(&{struct_name});")

        return lines

    def _build_espidf_pwm_config(
        self, categories: Dict[str, List[Dict[str, Any]]],
        symbol_map: Dict[int, str]
    ) -> List[str]:
        """Build ESP-IDF LEDC PWM configuration."""
        lines = []

        if not categories["pwm"]:
            return lines

        lines.append("    // PWM (LEDC)")

        # Timer config (one timer for all channels)
        lines.append("    ledc_timer_config_t ledc_timer = {")
        lines.append("        .speed_mode = LEDC_LOW_SPEED_MODE,")
        lines.append("        .duty_resolution = LEDC_TIMER_8_BIT,")
        lines.append("        .timer_num = LEDC_TIMER_0,")
        # Use speed_hz from first PWM pin if specified
        pwm_freq = 1000
        for pin in categories["pwm"]:
            speed = pin.get("speed_hz", 0)
            if speed and speed > 0:
                pwm_freq = speed
                break
        lines.append(f"        .freq_hz = {pwm_freq},")
        lines.append("        .clk_cfg = LEDC_AUTO_CLK,")
        lines.append("    };")
        lines.append("    ledc_timer_config(&ledc_timer);")
        lines.append("")

        # Channel config for each PWM pin (variant-aware)
        variant_lower = self.variant.lower()
        max_channels = 6 if ("c3" in variant_lower or "c6" in variant_lower) else 16
        for i, pin in enumerate(categories["pwm"][:max_channels]):
            gpio = pin.get("gpio", 0)
            const_name = symbol_map.get(gpio, f"PWM{gpio}")

            lines.append(f"    ledc_channel_config_t {const_name.lower()}_channel = {{")
            lines.append(f"        .gpio_num = {const_name}_PIN,")
            lines.append("        .speed_mode = LEDC_LOW_SPEED_MODE,")
            lines.append(f"        .channel = LEDC_CHANNEL_{i},")
            lines.append("        .timer_sel = LEDC_TIMER_0,")
            lines.append("        .duty = 0,")
            lines.append("        .hpoint = 0,")
            lines.append("    };")
            lines.append(f"    ledc_channel_config(&{const_name.lower()}_channel);")

        return lines

    def _build_espidf_adc_config(
        self, categories: Dict[str, List[Dict[str, Any]]]
    ) -> List[str]:
        """Build ESP-IDF ADC configuration."""
        lines = []

        if not categories["adc"]:
            return lines

        lines.append("    // ADC")
        lines.append("    adc1_config_width(ADC_WIDTH_BIT_12);")

        for pin in categories["adc"]:
            gpio = pin.get("gpio", 0)
            adc_info = self._get_adc_info(gpio)
            if adc_info and "ADC1" in adc_info:
                channel_num = adc_info.split("_CH")[1]
                lines.append(f"    adc1_config_channel_atten(ADC1_CHANNEL_{channel_num}, ADC_ATTEN_DB_11);")
            elif adc_info and "ADC2" in adc_info:
                channel_num = adc_info.split("_CH")[1]
                lines.append(f"    // ADC2 channel {channel_num} on GPIO{gpio} - configure at read time")
                lines.append(f"    // Note: ADC2 is unavailable when WiFi is active")

        return lines
