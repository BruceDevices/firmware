#include "core/USBSerial/USBSerial.h"
#include <cassert>
#include <iostream>

Stream Serial;

static void forwardArguments(SerialDevice &serial, const char *format, ...) {
    va_list args;
    va_start(args, format);
    serial.vprintf(format, args);
    va_end(args);
}

int main() {
    USBSerial adapter;
    SerialDevice &console = adapter;
    console.printf("Total: %u Free: %u Selected: %d\n", 16777216U, 12255232U, -1);
    assert(Serial.output == "Total: 16777216 Free: 12255232 Selected: -1\n");
    assert(Serial.printfCalls == 0 && Serial.vprintfCalls == 1);

    Serial.output.clear();
    console.printf("%s %04X %.2f %%", "image.bin", 0x2A, 3.25);
    assert(Serial.output == "image.bin 002A 3.25 %");

    Serial.output.clear();
    const std::string longText(256, 'x');
    forwardArguments(console, "%s:%u", longText.c_str(), 512U);
    assert(Serial.output == longText + ":512");
    assert(Serial.printfCalls == 0 && Serial.vprintfCalls == 3);

    Stream second;
    adapter.setSerialOutput(&second);
    assert(adapter.getSerialOutput() == &second);
    console.printf("%s %zu\n", "payload", size_t(1234));
    assert(second.output == "payload 1234\n");
    assert(second.printfCalls == 0 && second.vprintfCalls == 1);
    assert(Serial.vprintfCalls == 3);
    std::cout << "Actual USBSerial adapter: variadic formatting and output switching passed\n";
}
