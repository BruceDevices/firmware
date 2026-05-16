# ATTiny

@tableofcontents

ATTiny support for this library relies on the SpenceKonde ATTinyCore. Be sure to have added this core to the Arduino Boards Manager with [this install guide](http://highlowtech.org/?p=1695)

See the included rf24ping85 example for pin info and usage

@warning The ATTiny2313 is unsupported due to lack of sufficient memory resources

Some versions of Arduino IDE may require a patch to allow use of the full program space on ATTiny. See [this resource](https://github.com/TCWORLD/ATTinyCore/tree/master/PCREL%20Patch%20for%20GCC) for the ATTiny patch.

ATTiny board support initially added from [jscrane/RF24](https://github.com/jscrane/RF24)

## Hardware Configuration

By [tong67](https://github.com/tong67)

### ATtiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 4

```text
                             +-\/-+
               NC      PB5  1|o   |8  Vcc --- nRF24L01  VCC, pin2 --- LED --- 5V
nRF24L01  CE, pin3 --- PB3  2|    |7  PB2 --- nRF24L01  SCK, pin5
nRF24L01 CSN, pin4 --- PB4  3|    |6  PB1 --- nRF24L01 MOSI, pin6
nRF24L01 GND, pin1 --- GND  4|    |5  PB0 --- nRF24L01 MISO, pin7
                             +----+
```

### ATtiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 3 - PB3 and PB4 are free to use for application

- Circuit idea from [NerdRalph's 3 pin solution](http://nerdralph.blogspot.ca/2014/01/nrf24l01-control-with-3-attiny85-pins.html)
- Original RC combination was 1K/100nF. 22K/10nF combination worked better.
- For best settle-time delay value in `RF24::csn()` the timingSearch3pin.ino sketch can be used.
- This configuration is enabled when CE_PIN and CSN_PIN are equal, e.g. both 3
- Because CE is always high the power consumption is higher than for 5 pins solution

```text
                                                                                       ^^
                             +-\/-+           nRF24L01   CE, pin3 ------|              //
                       PB5  1|o   |8  Vcc --- nRF24L01  VCC, pin2 ------x----------x--|<|-- 5V
               NC      PB3  2|    |7  PB2 --- nRF24L01  SCK, pin5 --|<|---x-[22k]--|  LED
               NC      PB4  3|    |6  PB1 --- nRF24L01 MOSI, pin6  1n4148 |
nRF24L01 GND, pin1 -x- GND  4|    |5  PB0 --- nRF24L01 MISO, pin7         |
                    |        +----+                                       |
                    |-----------------------------------------------||----x-- nRF24L01 CSN, pin4
                                                                  10nF
```

### ATtiny24/44/84 Pin map with CE_PIN 8 and CSN_PIN 7

- Schematic provided and successfully tested by [Carmine Pastore](https://github.com/Carminepz)

```text
                              +-\/-+
nRF24L01  VCC, pin2 --- VCC  1|o   |14 GND --- nRF24L01  GND, pin1
                        PB0  2|    |13 AREF
                        PB1  3|    |12 PA1
                        PB3  4|    |11 PA2 --- nRF24L01   CE, pin3
                        PB2  5|    |10 PA3 --- nRF24L01  CSN, pin4
                        PA7  6|    |9  PA4 --- nRF24L01  SCK, pin5
nRF24L01 MISO, pin7 --- PA6  7|    |8  PA5 --- nRF24L01 MOSI, pin6
                              +----+
```

### ATtiny2313/4313 Pin map with CE_PIN 12 and CSN_PIN 13

```text
                              +-\/-+
                        PA2  1|o   |20 VCC --- nRF24L01  VCC, pin2
                        PD0  2|    |19 PB7 --- nRF24L01  SCK, pin5
                        PD1  3|    |18 PB6 --- nRF24L01 MOSI, pin6
                        PA1  4|    |17 PB5 --- nRF24L01 MISO, pin7
                        PA0  5|    |16 PB4 --- nRF24L01  CSN, pin4
                        PD2  6|    |15 PB3 --- nRF24L01   CE, pin3
                        PD3  7|    |14 PB2
                        PD4  8|    |13 PB1
                        PD5  9|    |12 PB0
nRF24L01  GND, pin1 --- GND 10|    |11 PD6
                              +----+
```
