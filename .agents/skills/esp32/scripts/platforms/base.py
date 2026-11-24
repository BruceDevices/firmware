"""
Platform abstraction layer for GPIO configuration validation and code generation.

This module defines the abstract base class and data models for the gpio-config
skill's validation and generation system. All platform-specific implementations
(Raspberry Pi, ESP32) inherit from the Platform base class defined here.

Python 3.9+ compatible. Uses only standard library modules.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, List, Optional


__all__ = [
    "Pin",
    "PeripheralGroup",
    "ConflictType",
    "ValidationResult",
    "GenerationResult",
    "Platform",
]


@dataclass
class Pin:
    """
    Represents a single GPIO pin on any supported platform.

    This data class encapsulates all properties of a GPIO pin including its
    electrical characteristics, capabilities, alternate functions, and usage
    restrictions. It provides a platform-agnostic representation that works
    for both Raspberry Pi (BCM numbering) and ESP32 GPIO numbering.

    Attributes:
        gpio_num: Platform GPIO number. For Raspberry Pi, this is the BCM
            (Broadcom) number (0-27). For ESP32, this is the GPIO number (0-39).
        name: Human-readable display name for the pin, e.g., "GPIO18", "D5".
            Used in user-facing output and wiring notes.
        physical_pin: Physical header position for platforms with standardized
            headers. For Raspberry Pi, this is the 40-pin header position (1-40).
            None for platforms without standard headers or for pins not exposed.
        voltage: Logic level voltage in volts. Typically 3.3 for both Raspberry Pi
            and ESP32. Used for level-shifting calculations.
        max_current_ma: Maximum source/sink current per pin in milliamps.
            Raspberry Pi: typically 16mA per pin, 50mA total.
            ESP32: typically 40mA per pin (20mA recommended).
        internal_pulls: List of available internal pull resistor configurations.
            Subset of ["up", "down", "none"]. ESP32 GPIO34-39 have no internal
            pulls and return ["none"] only.
        default_state: Pin state at boot/reset. One of:
            - "input_pullup": Input mode with internal pull-up enabled
            - "input_pulldown": Input mode with internal pull-down enabled
            - "input_floating": Input mode with no pull (high-impedance)
            - "output_low": Output mode driving low
        alternate_functions: Mapping of alternate function identifiers to their
            descriptions. For Raspberry Pi, keys are "ALT0" through "ALT5".
            For ESP32, keys are signal names like "HSPI_MOSI", "U2TXD".
        can_input: Whether the pin can be configured as a digital input.
            True for most pins on both platforms.
        can_output: Whether the pin can be configured as a digital output.
            False for ESP32 GPIO34-39 which are input-only.
        adc_channel: ADC channel identifier if the pin has ADC capability,
            e.g., "ADC1_CH0", "ADC2_CH4". None if no ADC capability.
        dac_channel: DAC channel identifier if the pin has DAC capability,
            e.g., "DAC1", "DAC2". None if no DAC capability. Only ESP32 has DAC.
        touch_channel: Touch sensing channel identifier if supported,
            e.g., "TOUCH0". None if no touch capability. ESP32-specific.
        pwm_capable: Whether the pin supports hardware PWM output.
            On Raspberry Pi, only specific pins have hardware PWM.
            On ESP32, all output-capable pins support PWM via LEDC.
        usability: Pin availability classification. One of:
            - "free": Available for general use without restrictions
            - "restricted": Usable but has caveats (strapping pins, boot state)
            - "reserved": Used by system, should not be reassigned (EEPROM I2C)
            - "unusable": Cannot be used (flash SPI, PSRAM pins)
        usability_reason: Human-readable explanation when usability is not "free".
            Examples: "HAT EEPROM I2C", "flash SPI bus", "strapping pin - boot mode".
        conflicts: List of conflict descriptions for this pin. Populated during
            validation when the pin is assigned in ways that conflict with other
            assignments or system requirements.
        notes: Free-text notes about the pin. May include platform-specific
            caveats, errata, or usage recommendations.

    Example:
        >>> pin = Pin(
        ...     gpio_num=18,
        ...     name="GPIO18",
        ...     physical_pin=12,
        ...     voltage=3.3,
        ...     max_current_ma=16.0,
        ...     internal_pulls=["up", "down", "none"],
        ...     default_state="input_floating",
        ...     alternate_functions={"ALT0": "PCM_CLK", "ALT4": "SPI1_CE0", "ALT5": "PWM0"},
        ...     can_input=True,
        ...     can_output=True,
        ...     adc_channel=None,
        ...     dac_channel=None,
        ...     touch_channel=None,
        ...     pwm_capable=True,
        ...     usability="restricted",
        ...     usability_reason="Conflicts with PCM/audio output",
        ...     conflicts=[],
        ...     notes="PWM0 on ALT5. Avoid if using audio."
        ... )
    """

    gpio_num: int
    name: str
    physical_pin: Optional[int]
    voltage: float
    max_current_ma: float
    internal_pulls: List[str]
    default_state: str
    alternate_functions: Dict[str, str]
    can_input: bool
    can_output: bool
    adc_channel: Optional[str]
    dac_channel: Optional[str]
    touch_channel: Optional[str]
    pwm_capable: bool
    usability: str
    usability_reason: Optional[str] = None
    conflicts: List[str] = field(default_factory=list)
    notes: str = ""

    def to_dict(self) -> Dict[str, Any]:
        """
        Convert the Pin to a plain dictionary suitable for JSON serialization.

        Returns:
            Dict containing all pin attributes with JSON-compatible types.
            Lists and dicts are preserved as-is. None values are included.

        Example:
            >>> pin.to_dict()
            {
                "gpio_num": 18,
                "name": "GPIO18",
                "physical_pin": 12,
                ...
            }
        """
        return {
            "gpio_num": self.gpio_num,
            "name": self.name,
            "physical_pin": self.physical_pin,
            "voltage": self.voltage,
            "max_current_ma": self.max_current_ma,
            "internal_pulls": self.internal_pulls,
            "default_state": self.default_state,
            "alternate_functions": self.alternate_functions,
            "can_input": self.can_input,
            "can_output": self.can_output,
            "adc_channel": self.adc_channel,
            "dac_channel": self.dac_channel,
            "touch_channel": self.touch_channel,
            "pwm_capable": self.pwm_capable,
            "usability": self.usability,
            "usability_reason": self.usability_reason,
            "conflicts": self.conflicts,
            "notes": self.notes,
        }


@dataclass
class PeripheralGroup:
    """
    Represents a bus or peripheral that claims multiple GPIO pins.

    Peripheral groups model hardware buses (SPI, I2C, UART) and other
    multi-pin peripherals (PWM channels, PCM audio). They track which
    pins are claimed by the peripheral and whether those assignments
    are hardware-fixed or software-remappable.

    Attributes:
        name: Identifier for the peripheral group, e.g., "SPI0", "I2C1",
            "UART0", "PWM0". Must be unique within a platform.
        protocol: Protocol type, one of: "spi", "i2c", "uart", "pwm",
            "onewire", "pcm", "can". Used for filtering and validation.
        pins: Mapping of signal names to GPIO numbers. Keys are signal
            identifiers like "MOSI", "MISO", "SCLK", "CS0" for SPI, or
            "SDA", "SCL" for I2C. Values are GPIO numbers.
        is_fixed: True if pin assignments are hardware-fixed and cannot
            be changed (e.g., Raspberry Pi SPI0 uses GPIO7-11 only).
            False if pins can be remapped in software (e.g., ESP32 allows
            any GPIO for most peripherals via GPIO matrix).
        enabled_by: Configuration directive needed to enable this peripheral.
            For Raspberry Pi, this is a config.txt line like "dtparam=spi=on"
            or "dtoverlay=i2c1". For ESP32, typically None since peripherals
            are enabled in code.
        notes: Free-text caveats or usage notes. May include information
            about conflicts, limitations, or configuration requirements.

    Example:
        >>> spi0 = PeripheralGroup(
        ...     name="SPI0",
        ...     protocol="spi",
        ...     pins={"MOSI": 10, "MISO": 9, "SCLK": 11, "CE0": 8, "CE1": 7},
        ...     is_fixed=True,
        ...     enabled_by="dtparam=spi=on",
        ...     notes="Active low chip enables. MOSI/MISO directly connected."
        ... )
    """

    name: str
    protocol: str
    pins: Dict[str, int]
    is_fixed: bool
    enabled_by: Optional[str]
    notes: str = ""

    def to_dict(self) -> Dict[str, Any]:
        """
        Convert the PeripheralGroup to a plain dictionary for JSON serialization.

        Returns:
            Dict containing all peripheral group attributes with JSON-compatible
            types. The pins dict is preserved with string keys and int values.

        Example:
            >>> spi0.to_dict()
            {
                "name": "SPI0",
                "protocol": "spi",
                "pins": {"MOSI": 10, "MISO": 9, "SCLK": 11, "CE0": 8, "CE1": 7},
                ...
            }
        """
        return {
            "name": self.name,
            "protocol": self.protocol,
            "pins": self.pins,
            "is_fixed": self.is_fixed,
            "enabled_by": self.enabled_by,
            "notes": self.notes,
        }


class ConflictType(Enum):
    """
    Enumeration of all conflict categories for GPIO pin validation.

    Conflicts are organized into three groups:
    - Universal: Apply to all platforms
    - RPi-specific: Raspberry Pi only
    - ESP32-specific: ESP32 variants only

    Each conflict type maps to a specific validation check and error/warning
    message. The string values are used as error codes in ValidationResult.

    Universal Conflicts:
        DIRECT_CONFLICT: Two assignments claim the same GPIO pin.
        PERIPHERAL_GROUP: Pin is claimed by an active peripheral group.
        ELECTRICAL_CURRENT: Total current draw exceeds platform budget.
        ELECTRICAL_VOLTAGE: Voltage level mismatch (e.g., 5V on 3.3V pin).
        MISSING_PULLUP: Protocol requires pull-up but none configured.

    RPi-Specific Conflicts:
        ALT_FUNCTION: Requested function not available on this pin's ALT modes.
        RESOURCE_CONFLICT: Hardware resource contention (e.g., PWM vs audio).
        RESERVED_PIN: Pin is reserved for system use (GPIO0/1 EEPROM).
        UART_BT_CONFLICT: UART0 conflicts with Bluetooth on Pi 3/4/Zero2W.

    ESP32-Specific Conflicts:
        STRAPPING_PIN: Pin is a strapping pin with boot-mode implications.
        FLASH_PIN: Pin is used for flash SPI (GPIO6-11).
        ADC2_WIFI: ADC2 channel unusable when WiFi is active.
        INPUT_ONLY: Output requested on input-only pin (GPIO34-39).
        PSRAM_PIN: Pin is used for PSRAM on WROVER modules (GPIO16-17).
        INVALID_GPIO: GPIO number is out of range or otherwise invalid for this variant.
    """

    # Universal conflicts
    DIRECT_CONFLICT = "DIRECT_CONFLICT"
    PERIPHERAL_GROUP = "PERIPHERAL_GROUP"
    ELECTRICAL_CURRENT = "ELECTRICAL_CURRENT"
    ELECTRICAL_VOLTAGE = "ELECTRICAL_VOLTAGE"
    MISSING_PULLUP = "MISSING_PULLUP"

    # RPi-specific conflicts
    ALT_FUNCTION = "ALT_FUNCTION"
    RESOURCE_CONFLICT = "RESOURCE_CONFLICT"
    RESERVED_PIN = "RESERVED_PIN"
    UART_BT_CONFLICT = "UART_BT_CONFLICT"

    # ESP32-specific conflicts
    STRAPPING_PIN = "STRAPPING_PIN"
    FLASH_PIN = "FLASH_PIN"
    ADC2_WIFI = "ADC2_WIFI"
    INPUT_ONLY = "INPUT_ONLY"
    PSRAM_PIN = "PSRAM_PIN"
    INVALID_GPIO = "INVALID_GPIO"


@dataclass
class ValidationResult:
    """
    Result of validating a GPIO pin assignment configuration.

    Encapsulates all errors, warnings, and summary statistics from running
    validation checks on a proposed pin assignment. The to_dict() method
    produces the exact JSON structure expected by the SKILL.md script interface.

    Attributes:
        valid: True if no errors were found (warnings are acceptable).
            False if any errors exist that prevent the configuration from
            being used safely.
        errors: List of error dictionaries. Each error dict contains:
            - gpio (int): The GPIO number where the error occurred
            - code (str): Error code from ConflictType enum value
            - message (str): Human-readable error description
            - severity (str): Always "error" for items in this list
        warnings: List of warning dictionaries. Each warning dict contains:
            - gpio (int): The GPIO number where the warning applies
            - code (str): Warning code from ConflictType enum value
            - message (str): Human-readable warning description
            - severity (str): Always "warning" for items in this list
        summary: Summary statistics dictionary containing:
            - total_pins (int): Number of pins in the assignment
            - errors (int): Count of error items
            - warnings (int): Count of warning items
            - current_draw_ma (float): Estimated total current draw in mA

    Example:
        >>> result = ValidationResult(
        ...     valid=True,
        ...     errors=[],
        ...     warnings=[{
        ...         "gpio": 12,
        ...         "code": "STRAPPING_PIN",
        ...         "message": "GPIO12 is MTDI strapping pin",
        ...         "severity": "warning"
        ...     }],
        ...     summary={
        ...         "total_pins": 8,
        ...         "errors": 0,
        ...         "warnings": 1,
        ...         "current_draw_ma": 32.0
        ...     }
        ... )
    """

    valid: bool
    errors: List[Dict[str, Any]]
    warnings: List[Dict[str, Any]]
    summary: Dict[str, Any]

    def to_dict(self) -> Dict[str, Any]:
        """
        Convert the ValidationResult to the exact JSON structure from SKILL.md.

        Returns:
            Dict with the following structure:
            {
                "valid": bool,
                "errors": [{"gpio": int, "code": str, "message": str, "severity": "error"}, ...],
                "warnings": [{"gpio": int, "code": str, "message": str, "severity": "warning"}, ...],
                "summary": {
                    "total_pins": int,
                    "errors": int,
                    "warnings": int,
                    "current_draw_ma": float
                }
            }

        Example:
            >>> result.to_dict()
            {
                "valid": true,
                "errors": [],
                "warnings": [{"gpio": 12, "code": "STRAPPING_PIN", ...}],
                "summary": {"total_pins": 8, "errors": 0, "warnings": 1, "current_draw_ma": 32}
            }
        """
        return {
            "valid": self.valid,
            "errors": self.errors,
            "warnings": self.warnings,
            "summary": self.summary,
        }


@dataclass
class GenerationResult:
    """
    Result of generating configuration and initialization code for a pin assignment.

    Encapsulates all generated artifacts including platform configuration lines,
    initialization code, wiring instructions, and alternative suggestions. The
    to_dict() method produces the exact JSON structure expected by SKILL.md.

    Attributes:
        config_lines: List of platform configuration directives.
            For Raspberry Pi: config.txt lines like "dtparam=i2c_arm=on".
            For ESP32 with ESP-IDF: sdkconfig.defaults lines.
            For ESP32 with Arduino: typically empty (config in code).
        init_code: Complete initialization code as a single string with
            embedded newlines. Language/framework depends on the generate_code
            call: Python for gpiozero/RPi.GPIO, C++ for Arduino/ESP-IDF.
        wiring_notes: List of human-readable wiring instructions. Each note
            describes a physical connection, e.g., "Connect BME280 SDA to
            GPIO2 (physical pin 3). Add 4.7kΩ pull-up to 3.3V."
        warnings: List of warning messages that apply to the overall
            configuration. Unlike ValidationResult warnings, these are
            simple strings rather than structured dicts.
        alternatives: List of alternative pin suggestions. Each dict contains:
            - original_gpio (int): The originally requested GPIO
            - alternative_gpio (int): Suggested replacement GPIO
            - reason (str): Why the alternative is recommended

    Example:
        >>> result = GenerationResult(
        ...     config_lines=["dtparam=i2c_arm=on", "dtparam=spi=on"],
        ...     init_code="from gpiozero import LED\\nled = LED(17)\\n",
        ...     wiring_notes=["Connect LED anode to GPIO17 via 330Ω resistor"],
        ...     warnings=[],
        ...     alternatives=[]
        ... )
    """

    config_lines: List[str]
    init_code: str
    wiring_notes: List[str]
    warnings: List[str]
    alternatives: List[Dict[str, Any]]

    def to_dict(self) -> Dict[str, Any]:
        """
        Convert the GenerationResult to the exact JSON structure from SKILL.md.

        Returns:
            Dict with the following structure:
            {
                "config_lines": ["dtparam=i2c_arm=on", ...],
                "init_code": "...complete script...",
                "wiring_notes": ["Connect BME280 SDA to GPIO2 (pin 3)", ...],
                "warnings": [],
                "alternatives": [
                    {"original_gpio": 18, "alternative_gpio": 12, "reason": "..."},
                    ...
                ]
            }

        Example:
            >>> result.to_dict()
            {
                "config_lines": ["dtparam=i2c_arm=on"],
                "init_code": "from gpiozero import LED...",
                "wiring_notes": [...],
                "warnings": [],
                "alternatives": []
            }
        """
        return {
            "config_lines": self.config_lines,
            "init_code": self.init_code,
            "wiring_notes": self.wiring_notes,
            "warnings": self.warnings,
            "alternatives": self.alternatives,
        }


class Platform(ABC):
    """
    Abstract base class for platform-specific GPIO implementations.

    This class defines the interface that all platform implementations must
    provide. Concrete subclasses (RaspberryPi, ESP32) implement the abstract
    methods to provide platform-specific pin databases, validation rules,
    and code generation.

    The Platform class is instantiated with platform identification parameters
    and provides methods to:
    - Query pin information and capabilities
    - Query peripheral group configurations
    - Validate proposed pin assignments
    - Generate platform configuration and initialization code

    Attributes:
        name: Platform family identifier, either "rpi" or "esp32".
        variant: Specific model/variant identifier. For Raspberry Pi:
            "rpi3", "rpi4", "rpi5", "rpi_zero2w". For ESP32: "esp32",
            "esp32s2", "esp32s3", "esp32c3", "esp32c6".
        module: Module type for ESP32, or None for Raspberry Pi.
            ESP32 module types: "WROOM", "WROVER". Affects reserved pins
            (WROVER reserves GPIO16-17 for PSRAM).

    Example:
        >>> # Concrete implementation would be used like this:
        >>> platform = RaspberryPi(variant="rpi4")
        >>> pin = platform.get_pin(18)
        >>> print(pin.name)
        GPIO18
        >>> result = platform.validate(assignment_dict)
        >>> if result.valid:
        ...     code = platform.generate_code(assignment_dict, "gpiozero")
    """

    def __init__(
        self,
        name: str,
        variant: str,
        module: Optional[str] = None
    ) -> None:
        """
        Initialize a Platform instance.

        Args:
            name: Platform family identifier ("rpi" or "esp32").
            variant: Specific model variant. Valid values depend on platform:
                - RPi: "rpi3", "rpi4", "rpi5", "rpi_zero2w"
                - ESP32: "esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6"
            module: Module type for ESP32 platforms. Valid values:
                - "WROOM": Standard flash-only module
                - "WROVER": Module with PSRAM (reserves GPIO16-17)
                - None: Use for Raspberry Pi or when module is unknown

        Raises:
            ValueError: If variant is not valid for the platform.
        """
        self.name = name
        self.variant = variant
        self.module = module

    @abstractmethod
    def get_pin(self, gpio_num: int) -> Pin:
        """
        Retrieve a specific pin by its GPIO number.

        Args:
            gpio_num: The GPIO number to look up. For Raspberry Pi, this is
                the BCM number (0-27). For ESP32, this is the GPIO number
                (0-39, varies by variant).

        Returns:
            Pin object containing all information about the requested pin.

        Raises:
            ValueError: If gpio_num is not a valid GPIO number for this
                platform/variant. The error message should indicate the
                valid range.

        Example:
            >>> pin = platform.get_pin(18)
            >>> print(pin.name, pin.usability)
            GPIO18 free
        """
        pass

    @abstractmethod
    def get_all_pins(self) -> List[Pin]:
        """
        Retrieve all GPIO pins available on this platform.

        Returns a complete list of all pins, regardless of usability status.
        This includes reserved, restricted, and unusable pins. Use
        get_usable_pins() to get only pins available for assignment.

        Returns:
            List of Pin objects for every GPIO on the platform. The list
            is ordered by GPIO number ascending.

        Example:
            >>> all_pins = platform.get_all_pins()
            >>> print(len(all_pins))
            28  # For Raspberry Pi
            >>> print(all_pins[0].gpio_num)
            0
        """
        pass

    def get_usable_pins(self) -> List[Pin]:
        """
        Retrieve all GPIO pins that are available for user assignment.

        Filters the complete pin list to include only pins where usability
        is "free" or "restricted". Excludes "reserved" (system use) and
        "unusable" (hardware-claimed) pins.

        This is a concrete method that calls get_all_pins() and filters.
        Subclasses should not override this unless they need custom
        filtering logic.

        Returns:
            List of Pin objects that can be assigned by users. Includes
            pins with restrictions (which generate warnings) but excludes
            pins that cannot be used at all.

        Example:
            >>> usable = platform.get_usable_pins()
            >>> for pin in usable:
            ...     print(pin.gpio_num, pin.usability)
            2 free
            3 free
            12 restricted  # strapping pin on ESP32
        """
        all_pins = self.get_all_pins()
        return [
            pin for pin in all_pins
            if pin.usability in ("free", "restricted")
        ]

    @abstractmethod
    def get_peripheral_group(self, name: str) -> PeripheralGroup:
        """
        Retrieve a peripheral group by its name.

        Args:
            name: The peripheral group identifier, e.g., "SPI0", "I2C1",
                "UART0", "PWM0". Case-sensitive.

        Returns:
            PeripheralGroup object containing pin assignments and metadata.

        Raises:
            ValueError: If no peripheral group with the given name exists
                on this platform. The error message should list available
                peripheral groups.

        Example:
            >>> spi = platform.get_peripheral_group("SPI0")
            >>> print(spi.pins)
            {'MOSI': 10, 'MISO': 9, 'SCLK': 11, 'CE0': 8, 'CE1': 7}
        """
        pass

    @abstractmethod
    def get_all_peripheral_groups(self) -> List[PeripheralGroup]:
        """
        Retrieve all peripheral groups defined for this platform.

        Returns:
            List of PeripheralGroup objects for all available peripherals.
            Includes both enabled and disabled peripherals. The list order
            is implementation-defined but typically grouped by protocol.

        Example:
            >>> groups = platform.get_all_peripheral_groups()
            >>> for g in groups:
            ...     print(g.name, g.protocol)
            SPI0 spi
            SPI1 spi
            I2C1 i2c
            UART0 uart
        """
        pass

    @abstractmethod
    def get_groups_for_protocol(self, protocol: str) -> List[PeripheralGroup]:
        """
        Retrieve all peripheral groups that use a specific protocol.

        Args:
            protocol: Protocol identifier to filter by. Valid values:
                "spi", "i2c", "uart", "pwm", "onewire", "pcm", "can".
                Case-sensitive, should be lowercase.

        Returns:
            List of PeripheralGroup objects matching the protocol.
            Empty list if no groups use the specified protocol.

        Example:
            >>> spi_groups = platform.get_groups_for_protocol("spi")
            >>> print([g.name for g in spi_groups])
            ['SPI0', 'SPI1']
        """
        pass

    @abstractmethod
    def validate(self, assignment: Dict[str, Any]) -> ValidationResult:
        """
        Validate a proposed GPIO pin assignment configuration.

        Checks the assignment for conflicts, constraint violations, and
        potential issues. Returns a ValidationResult indicating whether
        the configuration is valid and listing any errors or warnings.

        The assignment dict follows the input JSON schema from SKILL.md
        Section 6 (validate_pinmap.py input).

        Args:
            assignment: Pin assignment configuration dictionary with keys:
                - platform (str): "rpi" or "esp32"
                - variant (str): Platform variant identifier
                - module (str or None): ESP32 module type
                - wifi_enabled (bool): Whether WiFi is active (ESP32)
                - pins (list): List of pin assignment dicts, each with:
                    - gpio (int): GPIO number
                    - function (str): Assigned function
                    - protocol_bus (str): Protocol identifier (e.g., "i2c", "spi", "uart", "gpio")
                    - device (str): Device name
                    - pull (str): Pull configuration
                    - notes (str): Additional notes

        Returns:
            ValidationResult with valid flag, errors, warnings, and summary.

        Example:
            >>> assignment = {
            ...     "platform": "esp32",
            ...     "variant": "esp32",
            ...     "module": "WROOM",
            ...     "wifi_enabled": True,
            ...     "pins": [
            ...         {"gpio": 21, "function": "I2C_SDA", ...},
            ...         {"gpio": 22, "function": "I2C_SCL", ...}
            ...     ]
            ... }
            >>> result = platform.validate(assignment)
            >>> print(result.valid)
            True
        """
        pass

    @abstractmethod
    def generate_config(self, assignment: Dict[str, Any]) -> GenerationResult:
        """
        Generate platform configuration for a pin assignment.

        Produces configuration directives, wiring notes, and alternative
        suggestions for the given assignment. Does not generate code;
        use generate_code() for initialization code.

        Args:
            assignment: Pin assignment configuration dictionary (same
                format as validate() input).

        Returns:
            GenerationResult with config_lines, wiring_notes, warnings,
            and alternatives. The init_code field will be empty; call
            generate_code() to populate it.

        Example:
            >>> result = platform.generate_config(assignment)
            >>> print(result.config_lines)
            ['dtparam=i2c_arm=on']
            >>> print(result.wiring_notes[0])
            'Connect BME280 SDA to GPIO2 (physical pin 3)'
        """
        pass

    @abstractmethod
    def generate_code(self, assignment: Dict[str, Any], framework: str) -> str:
        """
        Generate initialization code for a pin assignment.

        Produces complete, runnable initialization code for the specified
        framework. The code includes pin mode setup, pull resistor config,
        and protocol initialization.

        Args:
            assignment: Pin assignment configuration dictionary (same
                format as validate() input).
            framework: Target framework identifier. Valid values depend
                on platform:
                - RPi: "gpiozero", "rpigpio" (for RPi.GPIO)
                - ESP32: "arduino", "espidf"

        Returns:
            Complete initialization code as a string with embedded newlines.
            Ready to be written to a file or executed.

        Raises:
            ValueError: If the framework is not supported for this platform.

        Example:
            >>> code = platform.generate_code(assignment, "gpiozero")
            >>> print(code)
            from gpiozero import LED, Button
            from gpiozero.pins.rpigpio import RPiGPIOFactory
            ...
        """
        pass
