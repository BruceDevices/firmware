# Serial adapter formatting regression

Run `python tests/serial_adapter/run.py`, optionally passing `--cxx /path/to/g++`.
This compiles the production `USBSerial.h` and `SerialDevice.h` unchanged against
a recording Arduino `Stream` stub. It checks integer, mixed, long and explicitly
forwarded `va_list` arguments, plus switching the output stream. No device,
PlatformIO build or external dependency is needed.

The former `out->printf(fmt, args)` forwarding fails these checks because it treats
the argument list itself as a variadic formatting argument. The corrected adapter
must call the framework's `Stream`/`Print::vprintf()` entry point.
