# Arduino

@tableofcontents

<!-- markdownlint-disable MD031 -->
RF24 is fully compatible with Arduino boards.

See [Arduino Board reference](http://www.arduino.cc/en/Reference/Board) and [Arduino SPI reference](http://arduino.cc/en/Reference/SPI) for more information

RF24 makes use of the standard hardware SPI pins (MISO, MOSI, SCK) and requires two additional pins, to control
the chip-select and chip-enable functions.

```cpp
RF24 radio(ce_pin, cs_pin);
```

These pins must be chosen and designated by the user and can use any
available pins.

## Alternate SPI Support

RF24 supports alternate SPI methods, in case the standard hardware SPI pins are otherwise unavailable.

### Software Driven SPI

Software driven SPI is provided by the [DigitalIO library](https://github.com/greiman/DigitalIO).

Setup:

1. Install the digitalIO library
2. Open RF24_config.h in a text editor.
   Uncomment the line
   ```cpp
   #define SOFTSPI
   ```
   or add the build flag/option
   ```shell
   -DSOFTSPI
   ```
3. In your sketch, add
   ```cpp
   #include <DigitalIO.h>
   ```

@note Note: Pins are listed as follows and can be modified by editing the RF24_config.h file.

```cpp
#define SOFT_SPI_MISO_PIN 16
#define SOFT_SPI_MOSI_PIN 15
#define SOFT_SPI_SCK_PIN 14
```

Or add the build flag/option

```shell
-DSOFT_SPI_MISO_PIN=16 -DSOFT_SPI_MOSI_PIN=15 -DSOFT_SPI_SCK_PIN=14
```

### Alternate Hardware (UART) Driven SPI

The Serial Port (UART) on Arduino can also function in SPI mode, and can double-buffer data, while the
default SPI hardware cannot.

The SPI_UART library is available at [TMRh20/Sketches](https://github.com/TMRh20/Sketches/tree/master/SPI_UART)

Enabling:

1. Install the SPI_UART library
2. Edit RF24_config.h and uncomment
   ```cpp
   #define SPI_UART
   ```
3. In your sketch, add
   ```cpp
   #include <SPI_UART.h>
   ```

SPI_UART SPI Pin Connections:
| NRF |Arduino Uno Pin|
|----:|:--------------|
| MOSI| TX(0) |
| MISO| RX(1) |
| SCK | XCK(4) |
| CE | User Specified|
| CSN | User Specified|

@note SPI_UART on Mega boards requires soldering to an unused pin on the chip. See [#24](https://github.com/TMRh20/RF24/issues/24) for more information on SPI_UART.

### Using a specific SPI Bus

An alternate SPI bus can be specified using the overloaded `RF24::begin(_SPI*)` method.
This is useful for some boards that offer more than 1 hardware-driven SPI bus or certain Arduino
cores that implement a software-driven (AKA bit-banged) SPI bus that does not use the DigitalIO
library.

@warning The SPI bus object's `SPIClass::begin()` method **must** be called before
calling the overloaded `RF24::begin(_SPI*)` method.

Below are some example snippets that demonstrate how this can be done.

#### ESP8266 example

@see The following example code is meant for the popular NodeMCU board. Please refer to the
[ESP8266 ArduinoCore's SPI documentation](https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#spi)
for [other ESP8266-based boards](https://arduino-esp8266.readthedocs.io/en/latest/boards.html#).

```cpp
#include <SPI.h>
#include <RF24.h>

// notice these pin numbers are not the same used in the library examples
RF24 radio(D4, D3); // the (ce_pin, csn_pin) connected to the radio

void setup() {
  Serial.begin(115200);
  while (!Serial) {} //some boards need this

  // by default (with no arguments passed) SPI uses D5 (HSCLK), D6 (HMISO), D7 (HMOSI)
  SPI.pins(6, 7, 8, 0);
  // this means the following pins are used for the SPI bus:
  // MOSI = SD1
  // MISO = SD0
  // SCLK = CLK
  // CSN = GPIO0 (labeled D3 on the board)
  // **notice we also passed `D3` to the RF24 constructor's csn_pin parameter**

  SPI.begin();

  if (!radio.begin(&SPI)) {
    Serial.println(F("radio hardware not responding!!"));
    while (1) {} // hold program in infinite loop to prevent subsequent errors
  }

  // ... continue with program as normal (see library examples/ folder)
}
```

#### ESP32 example

@see Please review the Espressif's
[SPI_Multiple_Buses.ino example for the ESP32](https://github.com/espressif/arduino-esp32/blob/master/libraries/SPI/examples/SPI_Multiple_Buses/SPI_Multiple_Buses.ino)
located in their ArduinoCore repository (along with the SPI library for the ESP32).

```cpp
#include <SPI.h>
#include <RF24.h>

// to use custom-defined pins, uncomment the following
// #define MY_MISO 26
// #define MY_MOSI 27
// #define MY_SCLK 25
// #define MY_SS   32  // pass MY_SS as the csn_pin parameter to the RF24 constructor

// notice these pin numbers are not the same used in the library examples
RF24 radio(2, 0); // the (ce_pin, csn_pin) connected to the radio

SPIClass* hspi = nullptr; // we'll instantiate this in the `setup()` function
// by default the HSPI bus pre-defines the following pins
// HSPI_MISO = 12
// HSPI_MOSI = 13
// HSPI_SCLK = 14
// HSPI_SS   = 15

void setup() {
  Serial.begin(115200);
  while (!Serial) {} //some boards need this

  hspi = new SPIClass(HSPI); // by default VSPI is used
  hspi->begin();
  // to use the custom defined pins, uncomment the following
  // hspi->begin(MY_SCLK, MY_MISO, MY_MOSI, MY_SS)

  if (!radio.begin(hspi)) {
    Serial.println(F("radio hardware not responding!!"));
    while (1) {} // hold program in infinite loop to prevent subsequent errors
  }

  // ... continue with program as normal (see library examples/ folder)
}
```

#### Teensy example

@see The overloaded RF24::begin(\_SPI\*) is not needed according to the
[Teensyduino SPI documentation](https://www.pjrc.com/teensy/td_libs_SPI.html).
Please review the table provided in the
[Teensyduino documentation](https://www.pjrc.com/teensy/td_libs_SPI.html) for what pins are used by
default for certain Teensy boards.

```cpp
#include <SPI.h>
#include <RF24.h>

// these pins are the alternate SPI pins available for Teensy LC/3.0/3.1/3.2/3.5/3.6
#define MY_MISO 8
#define MY_MOSI 7
#define MY_SCLK 14

// notice these pin numbers are not the same used in the library examples
RF24 radio(2, 0); // the (ce_pin, csn_pin) connected to the radio

void setup() {
  Serial.begin(115200);
  while (!Serial) {} //some boards need this

  SPI.setMOSI(MY_MOSI);
  SPI.setMISO(MY_MISO);
  SPI.setSCK(MY_SCLK);

  if (!radio.begin()) {
    Serial.println(F("radio hardware not responding!!"));
    while (1) {} // hold program in infinite loop to prevent subsequent errors
  }

  // ... continue with program as normal (see library examples/ folder)
}
```

<!--
#### MBED example
@see The [ArduinoCore-mbed SPI library](https://github.com/arduino/ArduinoCore-mbed/tree/master/libraries/SPI)
has predefined the possible hardware-driven SPI buses. This applies to Arduino Nano 33 BLE and
Arduino Portenta boards.

```cpp
#include <SPI.h>
#include <RF24.h>

RF24 radio(7, 8); // the (ce_pin, csn_pin) connected to the radio

void setup() {
  Serial.begin(115200);
  while (!Serial) {} //some boards need this

  SPI1.begin();

  if (!radio.begin(&SPI1)) {
    Serial.println(F("radio hardware not responding!!"));
    while (1) {} // hold program in infinite loop to prevent subsequent errors
  }

  // ... continue with program as normal (see library examples/ folder)
}
```

#### ATSAMD21 example
@see The Support for secondary SPI bus on ATSAMD21 chips is scarcely documented. However,
[Sparkfun has a tutorial about using a second SPI bus](https://learn.sparkfun.com/tutorials/adding-more-sercom-ports-for-samd-boards/adding-an-spi)
that often refers to the
[ArduinoCore-samd SPI library source code](https://github.com/arduino/ArduinoCore-samd/blob/master/libraries/SPI/SPI.h). This example applies Sparkfun's tutorial toward the RF24 library.
Special thanks to [ex-caliper](https://github.com/ex-caliper) for the lead!

```cpp
#include <SPI.h>
#include <RF24.h>
#include "wiring_private.h" // pinPeripheral()

// Define the pins are the alternate SPI pins we will use. These pin numbers ARE NOT chosen at random.
// The Sparkfun tutorial explains these choices in more detail (link is in "See Also" comment above).
#define MY_MISO 3
#define MY_MOSI 4
#define MY_SCLK 5

// instantiate the secondary SPI bus
SPIClass MY_SPI(&sercom2, MY_MISO, MY_SCLK, MY_MOSI, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_1);

// notice these pin numbers are not the same used in the library examples
RF24 radio(7, 6); // the (ce_pin, csn_pin) connected to the radio

void setup() {
  Serial.begin(115200);
  while (!Serial) {} //some boards need this

  // enable the alternate functionality for our secondary SPI bus' pins
  pinPeripheral(MY_MISO, PIO_SERCOM_ALT);
  pinPeripheral(MY_MOSI, PIO_SERCOM_ALT);
  pinPeripheral(MY_SCLK, PIO_SERCOM);

  MY_SPI.begin(); // initialize the secondary SPI bus

  if (!radio.begin(&MY_SPI)) {
    Serial.println(F("radio hardware not responding!!"));
    while (1) {} // hold program in infinite loop to prevent subsequent errors
  }

  // ... continue with program as normal (see library examples/ folder)
}
``` -->
