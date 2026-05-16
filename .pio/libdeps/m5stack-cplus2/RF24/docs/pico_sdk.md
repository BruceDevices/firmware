# Raspberry Pi Pico SDK (for the RP2040)

@tableofcontents

<!-- markdownlint-disable MD031 MD046 -->
Just to be very clear and concise: The following instructions are
**not required to use the Arduino IDE** with any RP2040-based boards.
These instructions pertain to using only [Raspberry Pi's Pico SDK](https://github.com/raspberrypi/pico-sdk)
for RP2040 based boards.

## Prerequisite

Follow Raspberry Pi's
["Getting Started with Pico" document](https://rptl.io/pico-get-started) to
setup a proper development environment on your host PC (the machine that
will build your project). Notice that the setup instructions are a little
different for building on a Windows PC.

Either set an environment variable named `PICO_SDK_PATH` that points to your
local clone of the pico-sdk or put the pico-sdk next to the RF24 folder or
next to the folder containing your project using the RF24 library:

    path/to/github/repos/
        pico-sdk/
        RF24/
        my_rf24_project/

Alternatively, the RF24 repository (and optionally the RF24Network and RF24Mesh
repositories) can be included into your project's "lib" folder as copies or
git submodules. For more detail, see the below instructions to incorporate
RF24 libs into your project.

## Building the RF24 examples for the Pico SDK

Each example imports a set of constants (`CE_PIN`, `CSN_PIN`, `IRQ_PIN`) from the
examples_pico/defaultPins.h file. Default values for SPI pins (SCK, MOSI, MISO)
come from the Pico SDK repository's
[pico-sdk/src/boards/include/boards/\<board_name>.h files](https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards)

1. Create a "build" directory in the RF24 repository's root directory and
   navigate to it:
   ```sh
   cd RF24
   mkdir build
   cd build
   ```
2. Configure CMake for your desired build type and specific RP2040-based board
   ```sh
   cmake ../examples_pico -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=pico
   ```
   Or if building on a Windows PC:
   ```sh
   cmake -G "NMake Makefiles" ../examples_pico -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=pico
   ```
   The supported RP2040-based boards are listed in header files in the Pico SDK
   repository's [pico-sdk/src/boards/include/boards/\<board_name>.h files](https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards).
   If the `-DPICO_BOARD` option is not specified, then the Pico SDK will default to building for the Raspberry Pi Pico board.
3. Build the examples using the CMakeLists.txt file located in the
   RF24/examples_pico directory.
   ```sh
   cmake --build . --config Release
   ```
   Notice we specified the build type again using the `--config` option.

   @note If you see an error stating "'PICO_DEFAULT_SPI_SCK_PIN' was not declared in this scope",
   then it means the board you selected with the `-DPICO_BOARD` option (in step 2) does not have a
   default set of SPI pins defined for it. To workaround this error, see the below instructions to
   use different pins for the SPI bus.

   @warning If doing consecutive build attempts, it is strongly encouraged to delete all files in the build
   directory before re-attempting to build the project.

## Incorporating RF24 libs into your project

In order to use the RF24 libraries in your RP2040 based project:

1. Make a copy of the RF24 library (and optionally RF24Network and RF24Mesh
   libraries) in a "lib" directory located in your project's root directory.

         path/to/my/project/
             lib/
                 RF24/
                 RF24Network/
                 RF24Mesh/
             src/
                 CMakeLists.txt
                 ...

   Alternatively you can add the RF24\* repositories as [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).
2. Include their CMakeLists.txt files from the RF24 libraries in your project's top-level
   CMakeLists.txt file (usually located in the "src" directory). The following snippet
   assumes that your project's "src" directory is on the same level as the previously
   mentioned "lib" directory.
   ```cmake
   include(../lib/RF24/CMakeLists.txt)
   include(../lib/RF24Network/CMakeLists.txt)
   include(../lib/RF24Mesh/CMakeLists.txt)
   ```
3. In the same CMakeLists.txt file from step 2, add the RF24 libraries into the
   `target_link_libraries` configuration:
   ```cmake
   target_link_libraries(${CMAKE_PROJECT_NAME}
       # ... Your project's other libraries ...
       RF24
       RF24Network
       RF24Mesh
   )
   ```
   If you are using tinyUSB, this line (or similar) should already exist:
   ```cmake
   target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
   ```
4. Finally, remember to include the necessary RF24\* libraries' header files in your
   project's source code where applicable.
   ```cpp
   #include <RF24.h>
   #include <RF24Network.h>
   #include <RF24Mesh.h>
   ```
   @note Any of the project's source file(s) that includes any of RF24\* libraries must be a C++ file.
   It is highly recommended to use the `.cpp` file extension for such project source files.

## Using different pins for the SPI bus

Initially (without modification), the SPI bus uses the default pins defined in the
Pico SDK repository's [pico-sdk/src/boards/include/boards/\<board_name>.h files](https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards).
However, there may be some boards that do not define the necessary pins to use as defaults. This can
be remedied using either project source code or build-time configuration.

@warning There is no support for software driven SPI on RP2040 based boards at this time.
If someone is so inclined to implement this using the Pico SDK's PIO (Programmable Input
Output) feature, please submit an issue or pull request to the
[RF24 repository](http://github.com/nRF24/RF24).

@note Before deciding what pins to use for the SPI bus, review the
[GPIO pins' "Function Select Table" in the Pico SDK documentation](https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__gpio.html#details).
There are essentially 2 SPI buses with multiple pin options for each.

### Project Source code option

This option is the most reliable and flexible. It involves calling `SPI.begin()` and then passing the `SPI` object to `RF24::begin(_SPI*)`.

```cpp
#include <RF24.h>

RF24 radio(7, 8); // pin numbers connected to the radio's CE and CSN pins (respectively)
SPI spi;

int main()
{
    // again please review the GPIO pins' "Function Select Table" in the Pico SDK docs
    spi.begin(spi0, 2, 3, 4); // spi0 or spi1 bus, SCK, TX, RX

    if (!radio.begin(&spi)) {
        printf("Radio hardware is not responding!\n");
    }
    // continue with program as normal ...
}
```

### Build-time configuration option

To specify the default SPI pins used at build time, you can use either:

1. declare these pins in the CMakeLists.txt file
   ```cmake
   target_compile_definitions(${CMAKE_PROJECT_NAME}
       PUBLIC PICO_DEFAULT_SPI=0 # can only be 0 or 1 (as in `spi0` or `spi1`)
       PUBLIC PICO_DEFAULT_SPI_SCK_PIN=2 # depends on which SPI bus (0 or 1) is being used
       PUBLIC PICO_DEFAULT_SPI_TX_PIN=3  # depends on which SPI bus (0 or 1) is being used
       PUBLIC PICO_DEFAULT_SPI_RX_PIN=4  # depends on which SPI bus (0 or 1) is being used
   )
   ```
2. additional command line arguments
   ```shell
   cmake --build . --config Release -DPICO_DEFAULT_SPI=0 -DPICO_DEFAULT_SPI_SCK_PIN=2 -DPICO_DEFAULT_SPI_TX_PIN=3 -DPICO_DEFAULT_SPI_RX_PIN=4
   ```

## Using Multiple Radios

It is possible to drive multiple nRF24L01 transceivers on a single board. To do this each radio needs dedicated digital output pins for the CE and CSN pins.

@warning The RPi Pico board's 3v regulator is typically insufficient to power more than 1 radio.
It is also worth mentioning that the RPi Pico board uses a switching regulator which inherently
produces electrical noise (a not steady 3v signal otherwise referred to as "power instability").

If you want to drive each radio with a separate SPI bus, then the following example will demonstrate how to do that.

```cpp
#include <RF24.h>

// Declare the pin numbers connected to the radios' CE and CSN pins (respectively)
RF24 radio0(8, 5);   // first radio object
RF24 radio1(14, 13); // second radio object

// By default, one SPI bus instance is created by the RF24 lib. We'll use this
// default instance of the `spi0` interface for our first radio, but we want a
// different SPI bus for the second radio.
// 
// So, here we declare a second SPI bus instance:
SPI my_spi; // we specify the `spi1` bus interface below

bool setupRadios()
{
    // Initialize the first radio using the default SPI instance
    if (!radio0.begin()) {
        printf("Radio0 hardware is not responding!\n");
        return false;
    }
    // first radio object initialized successfully

    // specify the the second SPI bus interface and corresponding GPIO pins
    my_spi.begin(spi1, 10, 11, 12); // spi1 bus, SCK, TX, RX
    if (!radio1.begin(&my_spi)) {
        printf("Radio1 hardware is not responding!\n");
        return false;
    }
    // second radio object initialized successfully

    return true;
}

int main()
{
    stdio_init_all(); // init necessary IO for the RP2040

    while (!setupRadios()) { // if either radioX.begin() failed
        sleep_ms(1000); // add 1 second delay for console readability
        // hold program in infinite attempts to initialize the radios
    }

    // continue with program as normal ...
}
```
