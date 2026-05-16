# RF24 Portability

@tableofcontents

The RF24 radio driver mainly utilizes the [Arduino API](http://arduino.cc/en/reference/homePage)
for GPIO, SPI, and timing functions, which are easily replicated on various platforms.

Support files for these platforms are stored under RF24/utility, and can be modified to provide the
required functionality.

## Basic Hardware Template

### RF24/utility

The RF24 library now includes a basic hardware template to assist in porting to various platforms.

The following files can be included to replicate standard Arduino functions as needed, allowing devices from ATTiny to Raspberry Pi to utilize the same core RF24 driver.

| File               | Purpose                                                                      |
| ------------------ | ---------------------------------------------------------------------------- |
| RF24_arch_config.h | Basic Arduino/AVR compatibility, includes for remaining support files, etc   |
| includes.h         | Linux only. Defines specific platform, include correct RF24_arch_config file |
| spi.h              | Provides standardized SPI (`transfer()`) methods                             |
| gpio.h             | Provides standardized GPIO (`digitalWrite()`) methods                        |
| compatibility.h    | Provides standardized timing (`millis()`, `delay()`) methods                 |
| your_custom_file.h | Provides access to custom drivers for spi, gpio, etc                         |

Examples are provided via the included hardware support templates in **RF24/utility**

See the [modules](modules.html) page for examples of class declarations.

## Device Detection

1. The main detection for Linux devices is done in the configure script, with the includes.h from the proper hardware directory copied to RF24/utility/includes.h
2. Secondary detection is completed in RF24_config.h, causing the include.h file to be included for all supported Linux devices
3. RF24.h contains the declaration for SPI and GPIO objects 'spi' and 'gpio' to be used for porting-in related functions.

## Code

To have your ported code included in this library, or for assistance in porting, create a pull request or open an issue at [RF24 Library issues](https://github.com/nRF24/RF24)
