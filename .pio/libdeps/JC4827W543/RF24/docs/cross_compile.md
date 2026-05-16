# Linux cross-compilation

@tableofcontents

<!-- markdownlint-disable MD031 -->
@warning These instructions are no longer recommended because they involve disabling security measures
for the target system. Please try the [instructions using CMake](using_cmake.md).

RF24 library supports cross-compilation. Advantages of cross-compilation:

- development tools don't have to be installed on target machine
- resources of target machine don't have to be sufficient for compilation
- compilation time can be reduced for large projects

Following prerequisites need to be assured:

- ssh password-less access to target machine [Here is a hint](https://linuxconfig.org/passwordless-ssh)
- sudo of a remote user without password [Here is a hint](http://askubuntu.com/questions/334318/sudoers-file-enable-nopasswd-for-user-all-commands)
- cross-compilation toolchain for your target machine; for RPi
  ```shell
  git clone https://github.com/raspberrypi/tools rpi_tools
  ```
  and cross-compilation tools must be in PATH, for example
  ```shell
  export PATH=$PATH:/your/dir/rpi-tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
  ```

## Cross compilation steps

1. clone RF24 to a machine for cross-compilation
   ```shell
   git clone https://github.com/nRF24/RF24
   cd RF24
   ```
2. configure for cross compilation
   ```shell
   ./configure --remote=pi@target_linux_host
   ```
   eventually
   ```shell
   ./configure --remote=pi@target_linux_host --driver=<driver>
   ```
3. build
   ```shell
   make
   ```
4. (optional) install library to cross-compilation machine into cross-environment - important for compilation of examples
   ```shell
   sudo make install
   ```
5. upload library to target machine
   ```shell
   make upload
   ```
6. (optional) compile examples
   ```shell
   cd examples_linux
   make
   ```
7. (optional) upload examples to target machine
   ```shell
   make upload
   ```
