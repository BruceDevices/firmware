# Linux Installation

@tableofcontents

Generic Linux devices are supported via SPIDEV, PiGPIO, MRAA, RPi native via BCM2835, or using LittleWire.

@note The SPIDEV option should work with most Linux systems supporting spi userspace device.


### Automatic Installation (New)

Using CMake: (See the [instructions using CMake](using_cmake.md) for more information and options)

1. Download the install.sh file from [https://github.com/nRF24/.github/blob/main/installer/install.sh](https://github.com/nRF24/.github/blob/main/installer/install.sh)
   ```shell
   wget https://raw.githubusercontent.com/nRF24/.github/main/installer/install.sh
   ```
2. Make it executable
   ```shell
   chmod +x install.sh
   ```
3. Run it and choose your options
   ```shell
   ./install.sh
   ```

   @warning 
   `SPIDEV` is now always selected as the default driver because
   all other Linux drivers are being removed in the future.
   See [RF24 issue #971](https://github.com/nRF24/RF24/issues/971) for rationale.

   It will also ask to install a python package named [pyRF24](https://github.com/nRF24/pyRF24).
   This is not the same as the traditionally provided python wrappers as the pyRF24 package can be
   used independent of the C++ installed libraries. For more information on this newer python
   package, please check out [the pyRF24 documentation](https://nrf24.github.io/pyRF24/).
4. Try an example from one of the libraries
   ```shell
   cd ~/rf24libs/RF24/examples_linux
   ```

   Edit the gettingstarted example, to set your pin configuration
   ```shell
   nano gettingstarted.cpp
   ```

   Build the examples. Remember to set the `RF24_DRIVER` option according to the one that was
   selected during the scripted install.
   ```shell
   mkdir build && cd build
   cmake .. -D RF24_DRIVER=SPIDEV
   make
   ```

   Run the example
   ```shell
   sudo ./gettingstarted
   ```

See the [instructions using CMake](using_cmake.md) for more information and options

## Automated Install (Deprecated - 32 bit only)

**Designed & Tested on RPi** - Defaults to SPIDEV on devices supporting it

1. Install prerequisites if there are any (pigpio, MRAA, LittleWire libraries, setup SPI device etc)
2. Download the install.sh file from [http://tmrh20.github.io/RF24Installer/RPi/install.sh](http://tmrh20.github.io/RF24Installer/RPi/install.sh)
   ```shell
   wget http://tmrh20.github.io/RF24Installer/RPi/install.sh
   ```
3. Make it executable
   ```shell
   chmod +x install.sh
   ```
4. Run it and choose your options
   ```shell
   ./install.sh
   ```

   @warning 
   `SPIDEV` is now always selected as the default driver because
   all other Linux drivers are being removed in the future.
   See [RF24 issue #971](https://github.com/nRF24/RF24/issues/971) for rationale.

5. Run an example from one of the libraries
   ```shell
   cd rf24libs/RF24/examples_linux
   ```
   Edit the gettingstarted example, to set your pin configuration
   ```shell
   nano gettingstarted.cpp
   make
   sudo ./gettingstarted
   ```

## Manual Install (Deprecated - 32 bit only)

1. Install prerequisites if there are any (pigpio, MRAA, LittleWire libraries, setup SPI device etc)
   @note See the [MRAA](http://iotdk.intel.com/docs/master/mraa/index.html) documentation for more info on installing MRAA
2. Make a directory to contain the RF24 and possibly RF24Network lib and enter it
   ```shell
   mkdir ~/rf24libs
   cd ~/rf24libs
   ```
3. Clone the RF24 repo
   ```shell
   git clone https://github.com/tmrh20/RF24.git RF24
   ```
4. Change to the new RF24 directory
   ```shell
   cd RF24
   ```
5. Configure build environment using the command:
   ```shell
   ./configure
   ```
   It automatically detects device and build environment.

   @warning 
   `SPIDEV` is now always selected as the default driver because
   all other Linux drivers are being removed in the future.
   See [RF24 issue #971](https://github.com/nRF24/RF24/issues/971) for rationale.

   For overriding automatic detections, use command-line arguments, see
   ```shell
   ./configure --help
   ```
   for description.
6. Build the library, and run an example file
   ```shell
   make
   sudo make install
   ```
   ```shell
   cd examples_linux
   ```
   Edit the gettingstarted example, to set your pin configuration
   ```shell
   ano gettingstarted.cpp
   make
   sudo ./gettingstarted
   ```

### Build using SPIDEV (Deprecated - 32 bit only)

1. Make sure that spi device support is enabled and /dev/spidev\<a\>.\<b\> is present
2. Manual Install using SPIDEV:
   ```shell
   ./configure --driver=SPIDEV
   make
   sudo make install
   ```
3. See the gettingstarted example for an example of pin configuration
