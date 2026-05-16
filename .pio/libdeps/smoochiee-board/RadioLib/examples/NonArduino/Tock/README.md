# RadioLib as Tock application

[Tock](https://github.com/tock/tock) is an embedded operating system designed
for running multiple concurrent, mutually distrustful applications on Cortex-M
and RISC-V based embedded platforms.

RadioLib can be built as a Tock application using
[libtock-c](https://github.com/tock/libtock-c). This is an example of running
RadioLib as a Tock application.

This has been tested on the
[SparkFun LoRa Thing Plus - expLoRaBLE board] (https://github.com/tock/tock/tree/master/boards/apollo3/lora_things_plus)
but will work on any LoRa compatible Tock board (currently only the
expLoRaBLE board).

libtock-c by default is bulit for RISC-V and ARM. RadioLib is also built
for both architectures by default. You can skip the RISC-V RadioLib build
by setting the `SKIP_RISCV` varaible.

The RadioLib example can be built with:

```shell
$ git clone https://github.com/jgromes/RadioLib.git
$ cd RadioLib/examples/NonArduino/Tock/
$ git clone https://github.com/tock/libtock-c.git
$ cd libtock-c; git checkout c0202f9ab78da4a6e95f136cf5250701e3778f63; cd ../
$ LIBTOCK_C_DIRECTORY="$(pwd)/libtock-c" ./build.sh
```

Then in the Tock repo you can flash the kernel and app with:

```shell
$ make flash; APP=RadioLib/examples/NonArduino/Tock/build-arm/tock-sx1261.tbf make flash-app
```
