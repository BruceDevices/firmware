/*
 * Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * Example using Dynamic Payloads
 *
 * This is an example of how to use payloads of a varying (dynamic) size.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// Hardware configuration
RF24 radio(7, 8);  // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t addresses[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

/************************* Role management ****************************/
// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.

// The role_pin is a digital input pin used to set the role of this radio.
// Connect the role_pin to GND to be the 'pong' receiver
// Leave the role_pin open to be the 'ping' transmitter
const short role_pin = 5;  // use pin 5
typedef enum { role_ping_out = 1,
               role_pong_back } role_e;                                     // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back" };  // The debug-friendly names of those roles
role_e role;                                                                // The role of the current running sketch


// variables used for changing the payload size dynamically (used when role == role_ping_out)
const int min_payload_size = 4;
const int max_payload_size = 32;
const int payload_size_increment = 1;
int send_payload_size = min_payload_size;

char receive_payload[max_payload_size + 1];  // +1 to allow room for a terminating NULL char

void setup(void) {
  pinMode(role_pin, INPUT);  // set up the role pin
  digitalWrite(role_pin, HIGH);
  delay(20);  // Just to get a solid reading on the role pin

  // read the role_pin, establish our role
  if (digitalRead(role_pin)) {
    role = role_ping_out;
  } else {
    role = role_pong_back;
  }

  Serial.begin(115200);
  printf_begin();  // needed for printDetails()

  // Print preamble
  Serial.println(F("RF24/examples/pingpair_dyn/"));
  Serial.print(F("ROLE: "));
  Serial.println(role_friendly_name[role]);

  // Setup and configure rf radio
  radio.begin();
  radio.enableDynamicPayloads();  // Enable dynamic payloads
  radio.setRetries(5, 15);        // delay between retries = 5 * 250 + 250 = 1500 microseconds, number of retries = 15

  // Open a writing and reading pipe on each radio, with opposite addresses
  if (role == role_ping_out) {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  } else {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  }

  radio.startListening();  // Start listening
  radio.printDetails();    // Dump the configuration of the rf unit for debugging
}

void loop() {


  /****************** Ping Out Role ***************************/

  if (role == role_ping_out) {
    // The payload will always be the same, what will change is how much of it we send.
    static char send_payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ789012";

    radio.stopListening();  // First, stop listening so we can talk.

    // Send the payload
    Serial.print(F("Now sending length "));
    Serial.println(send_payload_size);
    radio.write(send_payload, send_payload_size);  // This will block until complete

    radio.startListening();  // Now, continue listening

    unsigned long started_waiting_at = millis();  // Start a timer for measuring timeout
    bool timeout = false;
    while (!radio.available() && !timeout)  // Wait until we get a response or timeout is reached
    {
      if (millis() - started_waiting_at > 500)  // Only wait for 500 milliseconds
        timeout = true;
    }

    // Describe the results
    if (timeout) {
      Serial.println(F("Failed, response timed out."));
    } else {
      // Grab the response and print it

      uint8_t len = radio.getDynamicPayloadSize();  // get payload's length

      // If an illegal payload size was detected, all RX payloads will be flushed
      if (!len)
        return;

      radio.read(receive_payload, len);

      // Use payload as a C-string (for easy printing)
      receive_payload[len] = 0;  // put a NULL terminating zero at the end

      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);
    }

    send_payload_size += payload_size_increment;  // Update size for next time.
    if (send_payload_size > max_payload_size)     // if payload length is larger than the radio can handle
      send_payload_size = min_payload_size;       // reset the payload length

    delay(1000);  // Try again 1s later
  }


  /****************** Pong Back Role ***************************/
  // Receive each packet, send it back, and dump it out

  if (role == role_pong_back) {
    while (radio.available())  // if there is data ready
    {

      uint8_t len = radio.getDynamicPayloadSize();  // Fetch the the payload size

      // If an illegal payload size was detected, all RX payloads will be flushed
      if (!len)
        continue;

      radio.read(receive_payload, len);

      // Use payload as a C-string (for easy printing)
      receive_payload[len] = 0;  // put a NULL terminating zero at the end

      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);

      radio.stopListening();  // First, stop listening so we can talk

      // Send a reply that the packet was received
      //
      // You will have better luck delivering your message if
      // you wait for the other node to start listening first
      delay(20);
      radio.write(receive_payload, len);
      Serial.println(F("Sent response."));

      radio.startListening();  // Now, resume listening so we catch the next packets.
    }
  }
}  // loop
// vim:cin:ai:sts=2 sw=2 ft=cpp
