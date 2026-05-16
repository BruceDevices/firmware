/**
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * written in 2014 by tong67 (https://github.com/tong67)
 * Updated 2020 by 2bndy5 (http://github.com/2bndy5) for the
 * SpenceKonde ATTinyCore (https://github.com/SpenceKonde/ATTinyCore)
 */

/**
 * The RF24 library uses the [ATTinyCore by
 * SpenceKonde](https://github.com/SpenceKonde/ATTinyCore)
 *
 * This sketch is a duplicate of the ManualAcknowledgements.ino example
 * (without all the Serial input/output code), and it demonstrates
 * a ATTiny25/45/85 or ATTiny24/44/84 driving the nRF24L01 transceiver using
 * the RF24 class to communicate with another node.
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with manually transmitted (non-automatic) Acknowledgement (ACK) payloads.
 * This example still uses ACK packets, but they have no payloads. Instead the
 * acknowledging response is sent with `write()`. This tactic allows for more
 * updated acknowledgement payload data, where actual ACK payloads' data are
 * outdated by 1 transmission because they have to loaded before receiving a
 * transmission.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 */

/*
 * ********** Hardware configuration (& schematics) *******************
 *
 * When direct use of 3V does not work (UNO boards tend to have poor 3V supply),
 * use 5V with LED (1.8V ~ 2.2V drop) instead.
 * For low power consumption solutions floating pins (SCK and MOSI) should be
 * pulled HIGH or LOW with 10K resistors.
 *
 * ATTiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 4
 *                                                              ^^
 *                         +-\/-+                              //
 *                   PB5  1|o   |8  Vcc --- nRF24L01  VCC --- |<|--- 5V
 *  nRF24L01  CE --- PB3  2|    |7  PB2 --- nRF24L01  SCK     LED
 *  nRF24L01 CSN --- PB4  3|    |6  PB1 --- nRF24L01 MOSI
 *  nRF24L01 GND --- GND  4|    |5  PB0 --- nRF24L01 MISO
 *                         +----+
 *
 * ATTiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 3 => PB3 and PB4 are
 * free to use for other purposes. This "3 pin solution" is from
 * Ralph Doncaster (AKA NerdRalph) which is outlined on his blog at
 * http://nerdralph.blogspot.ca/2014/01/nrf24l01-control-with-3-attiny85-pins.html
 * Original RC combination was 1K/100nF. 22K/10nF combination worked better.
 *
 * For best settle time delay value to use for RF24::csDelay in RF24::csn(), use
 * the examples/rf24_ATTiny/timingSearch3pin/timingSearch3pin.ino sketch.
 *
 * This configuration is enabled in the RF24 library when CE_PIN and
 * CSN_PIN parameters to the constructor are equal. Notice (in the schematic
 * below) that these pins aren't directly to the ATTiny85. Because the CE pin
 * is always HIGH, the power consumption is higher than it would be for the
 * typical 5 pins solution.
 *                                                                              ^^
 *                         +-\/-+           nRF24L01 CE --------|              //
 *                   PB5  1|o   |8  Vcc --- nRF24L01 VCC -------x----------x--|<|-- 5V
 *                   PB3  2|    |7  PB2 --- nRF24L01 SCK ---|<|---x-[22k]--|  LED
 *                   PB4  3|    |6  PB1 --- nRF24L01 MOSI  1n4148 |
 *  nRF24L01 GND -x- GND  4|    |5  PB0 --- nRF24L01 MISO         |
 *                |        +----+                                 |
 *                |-----------------------------------------||----x-- nRF24L01 CSN
 *                                                         10nF
 *
 * ATTiny24/44/84 Pin map with CE_PIN 8 and CSN_PIN 7 & assuming 1.9V to 3V on VCC
 * Schematic provided and successfully tested by
 * Carmine Pastore (https://github.com/Carminepz)
 *
 *                          +-\/-+
 *  nRF24L01 VCC ---- VCC  1|o   |14 GND --- nRF24L01 GND
 *                    PB0  2|    |13 AREF
 *                    PB1  3|    |12 PA1
 *                    PB3  4|    |11 PA2 --- nRF24L01 CE
 *                    PB2  5|    |10 PA3 --- nRF24L01 CSN
 *                    PA7  6|    |9  PA4 --- nRF24L01 SCK
 *  nRF24L01 MOSI --- PA6  7|    |8  PA5 --- nRF24L01 MISO
 *                          +----+
 */

#include "SPI.h"
#include "RF24.h"

// CE and CSN are configurable, specified values for ATTiny85 as connected above
#define CE_PIN 3
#define CSN_PIN 4
//#define CSN_PIN 3 // uncomment for ATTiny85 3 pins solution

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1;  // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX node, false = RX node

// For this example, we'll be using a payload containing
// a string & an integer number that will be incremented
// on every successful transmission.
// Make a data structure to store the entire payload of different datatypes
struct PayloadStruct {
  char message[7];  // only using 6 characters for TX & RX payloads
  uint8_t counter;
};
PayloadStruct payload;

void setup() {

  // append a NULL terminating character for printing as a c-string
  payload.message[6] = 0;

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    while (1) {}  // hold in infinite loop
  }

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload));  // char[7] & uint8_t datatypes occupy 8 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1

  if (role) {
    // setup the TX node

    memcpy(payload.message, "Hello ", 6);  // set the outgoing message
    radio.stopListening();                 // put radio in TX mode
  } else {
    // setup the RX node

    memcpy(payload.message, "World ", 6);  // set the outgoing message
    radio.startListening();                // put radio in RX mode
  }
}  // setup()

void loop() {

  if (role) {
    // This device is a TX node

    bool report = radio.write(&payload, sizeof(payload));  // transmit & save the report

    if (report) {
      // transmission successful; wait for response and print results

      radio.startListening();                  // put in RX mode
      unsigned long start_timeout = millis();  // timer to detect no response
      while (!radio.available()) {             // wait for response or timeout
        if (millis() - start_timeout > 200)    // only wait 200 ms
          break;
      }
      radio.stopListening();  // put back in TX mode

      // print summary of transactions
      if (radio.available()) {  // is there a payload received?

        PayloadStruct received;
        radio.read(&received, sizeof(received));  // get payload from RX FIFO
        payload.counter = received.counter;       // save incoming counter for next outgoing counter
      }
    }  // report

    // to make this example readable in the serial monitor
    delay(1000);  // slow transmissions down by 1 second

  } else {
    // This device is a RX node

    if (radio.available()) {  // is there a payload?

      PayloadStruct received;
      radio.read(&received, sizeof(received));  // get incoming payload
      payload.counter = received.counter + 1;   // increment incoming counter for next outgoing response

      // transmit response & save result to `report`
      radio.stopListening();  // put in TX mode

      radio.writeFast(&payload, sizeof(payload));  // load response to TX FIFO
      radio.txStandBy(150);                        // keep retrying for 150 ms

      radio.startListening();  // put back in RX mode
    }
  }  // role
}  // loop
