# ATXMEGA

@tableofcontents

The RF24 driver can be build as a static library with Atmel Studio 7 in order to be included as any other library in another program for the XMEGA family.

Currently only the [ATXMEGA D3](https://www.microchip.com/wwwproducts/en/ATxmega64d3) family is implemented.

## Preparation

Create an empty GCC Static Library project in Atmel Studio 7.

As not all files are required, copy the following directory structure in the project:

```text
utility\
  ATXMegaD3\
    compatibility.c
    compatibility.h
    gpio.cpp
    gpio.h
    gpio_helper.c
    gpio_helper.h
    includes.h
    RF24_arch_config.h
    spi.cpp
    spi.h
nRF24L01.h
printf.h
RF24.cpp
RF24.h
RF24_config.h
```

## Usage

Add the library to your project!

In the file where the `main()` is put the following in order to update the millisecond functionality:

```cpp
ISR(TCE0_OVF_vect)
{
    update_milisec();
}
```

Declare the rf24 radio with `RF24 radio(XMEGA_PORTC_PIN3, XMEGA_SPI_PORT_C);`

1. First parameter is the CE pin which can be any available pin on the microcontroller.
2. Second parameter is the CS which can be on port C (**XMEGA_SPI_PORT_C**) or on port D (**XMEGA_SPI_PORT_D**).

Call the `__start_timer()` to start the millisecond timer.

@note The millisecond functionality is based on the TCE0 so don't use these pins as IO.

@note The operating frequency of the uC is 32MHz. If you have a different frequency, change the TCE0 registers appropriately in function `__start_timer()` in **compatibility.c** file for your frequency.
