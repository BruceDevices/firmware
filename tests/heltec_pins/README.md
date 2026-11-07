# Heltec fixed-pin configuration tests

After installing the `heltec-v4-expansion-v2` PlatformIO dependencies, run:

```sh
python tests/heltec_pins/run.py
```

Pass `--cxx /path/to/g++` if the host compiler is not on `PATH`, or `--arduinojson /path/to/ArduinoJson/src` for a different dependency location. Python 3 and a C++14 compiler are sufficient; the script builds temporary host executables and removes them afterwards.

The fixture compiles the actual `src/core/configPins.cpp` and its header with the board's actual pin definitions and ArduinoJson dependency. Only Arduino and filesystem I/O are stubbed. It exercises fixed SD/LoRa/GNSS/touch assignments, imported JSON, setter/save/load paths, invalid and reserved optional GPIOs, GPS baud, single-wire GPIO6 use, and the W5500 setter destination. Storage hooks assert that built-in pins are repaired before any filesystem access. A second build without the Heltec macro confirms that configurable buses remain mutable on other targets.

These checks do not validate electrical behavior, touchscreen input or RF operation.
