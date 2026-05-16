# XPowersLib Example

### Prerequisites

Please put XPowersLib and esp-idf in the same level directory, after configuring the esp-idf environment variable, enter `XPowersLib/examples/ESP_IDF_Example` and run the idf.py command directly


### Configure the Project

Open the project configuration menu (`idf.py menuconfig`).

In the `XPowers Configuration` menu:

* Select the PMU Type in the `PMU_Type` option.
* In `PMU SCL GPIO Num` select the clock pin to connect to the PMU,the default is 22
* In `PMU SDAGPIO Num` select the data pin connected to the PMU,the default is 21
* Select the interrupt pin connected to the PMU in `PMU Interrupt Pin`, the default is 35

## How to Use Example

Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.


### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

The output information is to configure the output voltage and enable status of the PMU

```
I (345) mian: I2C initialized successfully
I (355) AXP2101: Init PMU SUCCESS!
I (385) AXP2101: DCDC=======================================================================
I (385) AXP2101: DC1  :ENABLE    Voltage:3300 mV
I (385) AXP2101: DC2  :DISABLE   Voltage:900 mV
I (395) AXP2101: DC3  :ENABLE    Voltage:3300 mV
I (395) AXP2101: DC4  :DISABLE   Voltage:1100 mV
I (405) AXP2101: DC5  :DISABLE   Voltage:1200 mV
I (405) AXP2101: ALDO=======================================================================
I (415) AXP2101: ALDO1:ENABLE    Voltage:1800 mV
I (425) AXP2101: ALDO2:ENABLE    Voltage:2800 mV
I (425) AXP2101: ALDO3:ENABLE    Voltage:3300 mV
I (435) AXP2101: ALDO4:ENABLE    Voltage:3000 mV
I (435) AXP2101: BLDO=======================================================================
I (445) AXP2101: BLDO1:ENABLE    Voltage:3300 mV
```

## Build process example

Assuming you don't have esp-idf yet

```
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
git clone https://github.com/lewisxhe/XPowersLib.git
cd esp-idf
./install.sh
. ./export.sh
cd ..
cd XPowersLib/examples/ESP_IDF_Example
idf.py menuconfig
idf.py build
idf.py -b 921600 flash
idf.py monitor

```