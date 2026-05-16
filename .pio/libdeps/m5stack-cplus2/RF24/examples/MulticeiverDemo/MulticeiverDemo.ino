/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty 2bndy5
 */

/**
 * A simple example of sending data from as many as 6 nRF24L01 transceivers to
 * 1 receiving transceiver. This technique is trademarked by
 * Nordic Semiconductors as "MultiCeiver".
 *
 * This example was written to be used on up to 6 devices acting as TX nodes &
 * only 1 device acting as the RX node (that's a maximum of 7 devices).
 * Use the Serial Monitor to change each node's behavior.
 */
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

// For this example, we'll be using 6 addresses; 1 for each TX node
// It is very helpful to think of an address as a path instead of as
// an identifying device destination
// Notice that the last byte is the only byte that changes in the last 5
// addresses. This is a limitation of the nRF24L01 transceiver for pipes 2-5
// because they use the same first 4 bytes from pipe 1.
uint64_t address[6] = { 0x7878787878LL,
                        0xB3B4B5B6F1LL,
                        0xB3B4B5B6CDLL,
                        0xB3B4B5B6A3LL,
                        0xB3B4B5B60FLL,
                        0xB3B4B5B605LL };

// role variable is used to control whether this node is sending or receiving
char role = 'R';  // integers 0-5 = TX node; character 'R' or integer 82 = RX node

// For this example, we'll be using a payload containing
// a node ID number and a single integer number that will be incremented
// on every successful transmission.
// Make a data structure to use as a payload.
struct PayloadStruct {
  unsigned long nodeID;
  unsigned long payloadID;
};
PayloadStruct payload;

// This example uses all 6 pipes to receive while TX nodes only use 2 pipes
// To make this easier we'll use a function to manage the addresses, and the
// payload's nodeID
void setRole();  // declare a prototype; definition is found after the loop()

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  // print example's introductory prompt
  Serial.println(F("RF24/examples/MulticeiverDemo"));
  Serial.println(F("*** Enter a number between 0 and 5 (inclusive) to change"));
  Serial.println(F("    the identifying node number that transmits."));

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity of
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload));  // 2x int datatype occupy 8 bytes

  // Set the pipe addresses accordingly. This function additionally also
  // calls startListening() or stopListening() and sets the payload's nodeID
  setRole();

  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data

}  // setup()

void loop() {

  if (role <= 53) {
    // This device is a TX node

    unsigned long start_timer = micros();                  // start the timer
    bool report = radio.write(&payload, sizeof(payload));  // transmit & save the report
    unsigned long end_timer = micros();                    // end the timer

    if (report) {
      // payload was delivered

      Serial.print(F("Transmission of payloadID "));
      Serial.print(payload.payloadID);  // print payloadID
      Serial.print(F(" as node "));
      Serial.print(payload.nodeID);  // print nodeID
      Serial.print(F(" successful!"));
      Serial.print(F(" Time to transmit: "));
      Serial.print(end_timer - start_timer);  // print the timer result
      Serial.println(F(" us"));
    } else {
      Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    }
    payload.payloadID++;  // increment payload number

    // to make this example readable in the serial monitor
    delay(1000);  // slow transmissions down by 1 second

  } else if (role == 'R') {
    // This device is the RX node

    uint8_t pipe;
    if (radio.available(&pipe)) {              // is there a payload? get the pipe number that received it
      uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
      radio.read(&payload, bytes);             // fetch payload from FIFO
      Serial.print(F("Received "));
      Serial.print(bytes);  // print the size of the payload
      Serial.print(F(" bytes on pipe "));
      Serial.print(pipe);  // print the pipe number
      Serial.print(F(" from node "));
      Serial.print(payload.nodeID);  // print the payload's origin
      Serial.print(F(". PayloadID: "));
      Serial.println(payload.payloadID);  // print the payload's number
    }
  }  // role

  if (Serial.available()) {
    // change the role via the serial monitor

    char c = Serial.read();
    if (toupper(c) == 'R' && role <= 53) {
      // Become the RX node

      role = 'R';
      Serial.println(F("*** CHANGING ROLE TO RECEIVER ***"));
      Serial.println(F("--- Enter a number between 0 and 5 (inclusive) to act as"));
      Serial.println(F("    a unique node number that transmits to the RX node."));
      setRole();  // change address on all pipes to TX nodes

    } else if (c >= 48 && c <= 53 && c != role) {
      // Become a TX node with identifier 'c'

      role = c - 48;
      Serial.print(F("*** CHANGING ROLE TO NODE "));
      Serial.print(c);
      Serial.println(F(" ***"));
      Serial.println(F("--- Enter a number between 0 and 5 (inclusive) to change"));
      Serial.println(F("    the identifying node number that transmits."));
      Serial.println(F("--- PRESS 'R' to act as the RX node."));
      setRole();  // change address on pipe 0 to the RX node
    }
  }

}  // loop

void setRole() {
  if (role == 'R') {
    // For the RX node

    // Set the addresses for all pipes to TX nodes
    for (uint8_t i = 0; i < 6; ++i)
      radio.openReadingPipe(i, address[i]);

    radio.startListening();  // put radio in RX mode

  } else {
    // For the TX node

    // set the payload's nodeID & reset the payload's identifying number
    payload.nodeID = role;
    payload.payloadID = 0;

    // Set the address on pipe 0 to the RX node.
    radio.stopListening();  // put radio in TX mode
    radio.openWritingPipe(address[role]);

    // According to the datasheet, the auto-retry features's delay value should
    // be "skewed" to allow the RX node to receive 1 transmission at a time.
    // So, use varying delay between retry attempts and 15 (at most) retry attempts
    radio.setRetries(((role * 3) % 12) + 3, 15);  // maximum value is 15 for both args
  }
}  // setRole
