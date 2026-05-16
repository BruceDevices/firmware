# Linux General/Raspberry Pi

@tableofcontents

<!-- markdownlint-disable MD031 -->
RF24 supports a variety of Linux based devices via various drivers. Some boards like RPi can utilize multiple methods
to drive the GPIO and SPI functionality.

## Potential PreConfiguration

If SPI is not already enabled, load it on boot:

```shell
sudo raspi-config
```

1. Update the tool via the menu as required
2. Select **Advanced** and **enable the SPI kernel module**
3. Update other software and libraries
   ```shell
   sudo apt-get update
   sudo apt-get upgrade
   ```

## Build Options

The default build on Raspberry Pi utilizes the included **BCM2835** driver from [the BCM2835 Library](http://www.airspayce.com/mikem/bcm2835)

1. See [the Linux section for automated installation](linux_install.md).
2. Manual install:
   ```shell
   make
   sudo make install
   ```

## Connections and Pin Configuration

Using pin 15(GPIO22) for CE, pin 24(GPIO8 commonly labeled as CE0) for CSN

Can use any available SPI BUS for CSN.

In general, use

```cpp
RF24 radio(<ce_pin>, <a>*10+<b>);
```

for proper constructor to address the correct spi device at /dev/spidev\<a\>.\<b\>

Choose any GPIO output pin for radio CE pin.

### General

```cpp
RF24 radio(22,0);
```

### MRAA Constructor

```cpp
RF24 radio(15,0);
```

See [the MRAA documentation for Raspberry Pi support](http://iotdk.intel.com/docs/master/mraa/rasppi.html)

### SPI_DEV Constructor

```cpp
RF24 radio(22, 0);
```

See [the Raspberry Pi documentation about the GPIO pins](https://www.raspberrypi.com/documentation/computers/os.html#gpio-and-the-40-pin-header)

### Pins

| PIN | NRF24L01 | RPI        | RPi -P1 Connector |
| --- | -------- | ---------- | ----------------- |
| 1   | GND      | rpi-gnd    | (25)              |
| 2   | VCC      | rpi-3v3    | (17)              |
| 3   | CE       | rpi-gpio22 | (15)              |
| 4   | CSN      | rpi-gpio8  | (24)              |
| 5   | SCK      | rpi-sclk   | (23)              |
| 6   | MOSI     | rpi-mosi   | (19)              |
| 7   | MISO     | rpi-miso   | (21)              |
| 8   | IRQ      | -          | -                 |

## brief history of RF24 library lineage

Based on the arduino lib from [J. Coliz](maniacbug@ymail.com),
the library was berryfied by [Purinda Gunasekara](purinda@gmail.com)
then forked from github [stanleyseow/RF24](https://github.com/stanleyseow/RF24) to [jscrane/RF24-rpi](https://github.com/jscrane/RF24-rpi)

Network lib also based on [farconada/RF24Network](https://github.com/farconada/RF24Network)
