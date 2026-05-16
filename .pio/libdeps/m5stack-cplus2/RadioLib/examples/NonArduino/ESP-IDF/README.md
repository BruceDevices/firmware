# RadioLib ESP-IDF example

This example shows how to use RadioLib as ESP-IDF component, without the Arduino framework. To run in ESP-IDF (or on any other platform), RadioLib includes an internal hardware abstraction layer (HAL). This software layer takes care of basic interaction with the hardware, such as performing SPI transaction or GPIO operations. To run on your chosen ESP microcontroller, you will likely have to customize the example HAL for your specific ESP type.

## Structure of the example

* `main/CMakeLists.txt` - IDF component CMake file
* `main/EspHal.h` - RadioLib HAL example implementation
* `main/idf_component.yml` - declaration of the RadioLib dependency for this example
* `main/main.cpp` - the example source code
* `sdkconfig.defaults` - List of preset configuration option values for ESP-IDF. All other options use default values provided by ESP-IDF.
