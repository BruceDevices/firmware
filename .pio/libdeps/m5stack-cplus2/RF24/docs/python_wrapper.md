# Python Wrapper

@tableofcontents

@remark
@parblock
We recommend using the newer [pyRF24 package](https://github.com/nRF24/pyRF24)
[available from pypi](https://pypi.org/project/pyrf24/) because

1. it is [practically drop-in compatible](https://nrf24.github.io/pyRF24/#migrating-to-pyrf24)
2. easier to install or get updates with popular package managers like pip
3. does not require the C++ libraries to be installed -- it uses its own isolated binaries
4. includes wrappers for RF24, RF24Network, RF24Mesh libraries
5. includes a new [fake BLE implementation](https://nrf24.github.io/pyRF24/ble_api.html)
6. has its own [dedicated documentation](https://nRF24.github.io/pyRF24)
7. is compatible with python's builtin `help()`
8. includes typing stub files for type checking tools like mypy

The only reason that you should need to keep using these older individual python
wrappers is if you must use python v3.6 or older.

You **cannot** use these individual wrappers in combination with the pyRF24 package.
@endparblock

## Python Wrapper Prerequisites

These instructions work for the RF24, RF24Network, and RF24Mesh libraries, but
the C++ source code needs to be built and installed for the corresponding
python wrapper(s) to work.

@see Review [installing with CMake](using_cmake.md) and [Linux/RPi General](rpi_general.md).

@note The interrupt_configure.py example uses the
[gpiod library](https://pypi.org/project/gpiod) to watch the radio's IRQ pin.

> [!warning]
> Recent updates to pip mandate that users manage a python virtual environment.
> This is good practice because it avoids the risk that installing a package with pip will
> mess up a system-installed package.
>
> For information about using up a python virtual environment, see the official
> [instructions using the python standard library `venv`](https://packaging.python.org/en/latest/guides/installing-using-pip-and-virtual-environments/)
> (which is a minimized variant of
> [the `virtualenv` library](https://virtualenv.pypa.io/en/stable/index.html)).
>
> There's also a note at the end of these instructions that talks about
> using `sudo` within a python virtual environment.

### Python2

```shell
sudo apt-get install python-dev libboost-python-dev python-pip
```

Next, install some up-to-date python packages.

```shell
python -m pip install --upgrade pip setuptools
```

### Python3

```shell
sudo apt-get install python3-dev libboost-python-dev python3-pip
```

## Installation

> [!note]
> If using **older versions of RF24* libraries** with python3,
> the setup.py script needed a manually created symlink for the boost.python library.
> ```shell
> sudo ln -s $(ls /usr/lib/$(ls /usr/lib/gcc | tail -1)/libboost_python3*.so | tail -1) /usr/lib/$(ls /usr/lib/gcc | tail -1)/libboost_python3.so
> ```
> This is no longer needed with the latest versions of RF24 libraries.

1. Install the library.

   This step needs to be executed from the appropriate directory of
   the cloned RF24* repository:
   - navigate to *pyRF24* directory in the RF24 cloned repository
   - navigate to *RPi/pyRF24Network* directory in the RF24Network cloned repository
   - navigate to *pyRF24Mesh* directory in the RF24Mesh cloned repository

   When in the correct directory, run the following command:
   ```shell
   python3 -m pip install -v .
   ```
   @note Building/installing takes several minutes on arm-based machines.
   Machines with RAM less than 1GB may need to increase amount of swap for build.
   The `-v` option enables pip's verbose output to show that the process has not frozen.

   See the additional [Platform Support pages](pages.html) for information on connecting your hardware.

   See the included [\*.py files in the "examples_linux" folder](examples.html) for usage information.
2. Running the Example

   The python examples location differ for each RF24* repositories.
   - navigate to *examples_linux* directory in the RF24 cloned repository
   - navigate to *RPi/pyRF24Network/examples* directory in the RF24Network cloned repository
   - navigate to *examples_RPi* directory in the RF24Mesh cloned repository

   Navigate to the examples_linux directory
   ```shell
   cd ../examples_linux
   ```
   Edit the getting_started.py example to configure the appropriate pins per the [Linux/RPi General](rpi_general.md) documentation.
   ```shell
   nano getting_started.py
   ```
   Configure another device, Arduino or RPi with the same example code. It could be C++ or python (see the [list of available examples](examples.html)), but we'll use the same example file on a different device in this tutorial.

   Run the example
   ```shell
   python getting_started.py
   ```
   or for python3
   ```shell
   python3 getting_started.py
   ```

   @note
   @parblock
   Running the python wrappers built with 'pigpio' or 'RPi' drivers requires `sudo` permission.
   
   If you are working in a python virtual environment (aka "venv"), then the
   virtual environment's python executable must be specified after `sudo`. Otherwise,
   `sudo` may invoke the system-installed python executable which can lead to errors.

   Assuming the python virtual environment is located in `~/venv`, use the following command:
   ```
   sudo ~/venv/bin/python getting_started.py
   ```
   This `sudo` advice must be observed even while the virtual environment is activated.

   See more information about python virtual environments in the
   [python documentation](https://docs.python.org/3/library/venv.html).
   @endparblock
