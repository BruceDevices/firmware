/*
  Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.

  Update 2014 - TMRh20
*/

/**
  Example of using interrupts

  This is an example of how to user interrupts to interact with the radio, and a demonstration
  of how to use them to sleep when receiving, and not miss any payloads.
  The pingpair_sleepy example expands on sleep functionality with a timed sleep option for the transmitter.
  Sleep functionality is built directly into my fork of the RF24Network library
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// Hardware configuration
RF24 radio(7, 8);  // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

// Our ACK payload will simply be 4 bytes containing the number of payloads received
static uint32_t message_count = 1;  // start counting at 1

// Demonstrates another method of setting up the addresses
byte address[][5] = { 0xCC, 0xCE, 0xCC, 0xCE, 0xCC, 0xCE, 0xCC, 0xCE, 0xCC, 0xCE };

/************************* Role management ****************************/
// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.

// The role_pin is a digital input pin used to set the role of this radio.
// Connect the role_pin to GND to be the 'pong' receiver
// Leave the role_pin open to be the 'ping' transmitter
const short role_pin = 5;  // use pin 5
typedef enum { role_sender = 1,
               role_receiver } role_e;                                   // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Sender", "Receiver" };  // The debug-friendly names of those roles
role_e role;                                                             // The role of the current running sketch


void setup() {

  pinMode(role_pin, INPUT);      // set up the role pin
  digitalWrite(role_pin, HIGH);  // Change this to LOW/HIGH instead of using an external pin
  delay(20);                     // Just to get a solid reading on the role pin

  if (digitalRead(role_pin))  // read the role_pin pin to establish our role
    role = role_sender;
  else
    role = role_receiver;


  Serial.begin(115200);
  printf_begin();  // needed for printDetails()

  // print introduction
  Serial.print(F("\n\rRF24/examples/pingpair_irq\n\rROLE: "));
  Serial.println(role_friendly_name[role]);


  /********************** Setup and configure rf radio *********************/
  radio.begin();

  // Examples are usually run with both radios in close proximity to each other
  radio.setPALevel(RF24_PA_LOW);  // defaults to RF24_PA_MAX
  radio.enableAckPayload();       // We will be using the ACK Payload feature which is not enabled by default
  radio.enableDynamicPayloads();  // Ack payloads are dynamic payloads

  // Open a writing and reading pipe on each radio, with opposite addresses
  if (role == role_sender) {
    radio.openWritingPipe(address[0]);
    radio.openReadingPipe(1, address[1]);
  } else {
    radio.openWritingPipe(address[1]);
    radio.openReadingPipe(1, address[0]);
    radio.startListening();  // First we need to start listening

    // Add an ACK payload for the first time around; 1 is the pipe number to acknowledge
    radio.writeAckPayload(1, &message_count, sizeof(message_count));
    ++message_count;  // increment counter by 1 for next ACK payload
  }

  radio.printDetails();  // Dump the configuration of the rf unit for debugging
  delay(50);

  // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
  attachInterrupt(0, check_radio, LOW);
}  // setup


void loop() {


  /****************** Ping Out Role ***************************/

  if (role == role_sender) {
    // Repeatedly send the current time

    unsigned long time = millis();  // Take the time
    Serial.print(F("Now sending "));
    Serial.println(time);
    radio.startWrite(&time, sizeof(unsigned long), 0);  // Send the time
    delay(2000);                                        // Try again soon (in 2 seconds)
  }


  /****************** Pong Back Role ***************************/
  // Receiver does nothing!  All the work is in Interrupt Handler

  if (role == role_receiver) {}

}  // loop


/********************** Interrupt Handler *********************/

void check_radio(void) {

  bool tx, fail, rx;                 // declare variables to store IRQ flags
  radio.whatHappened(tx, fail, rx);  // What happened?

  if (tx) {  // Have we successfully transmitted?
    if (role == role_sender)
      Serial.println(F("Send:OK"));
    if (role == role_receiver)
      Serial.println(F("Ack Payload:Sent"));
  }

  if (fail) {  // Have we failed to transmit?
    if (role == role_sender)
      Serial.println(F("Send:Failed"));
    if (role == role_receiver)
      Serial.println(F("Ack Payload:Failed"));
  }

  if (rx || radio.available()) {  // Did we receive a message?




    /**************** Ping Out Role (about received ACK payload) ************************/
    // If we're the sender, we've received an ack payload
    if (role == role_sender) {
      // Get the payload and dump it
      radio.read(&message_count, sizeof(message_count));
      Serial.print(F("Ack: "));
      Serial.println(message_count);
    }


    /****************** Pong Back Role ***************************/
    // If we're the receiver, we've received a time message
    if (role == role_receiver) {
      // Get the payload and dump it

      static unsigned long got_time;            // variable to hold the received time
      radio.read(&got_time, sizeof(got_time));  // get the payload
      Serial.print(F("Got payload "));
      Serial.println(got_time);

      // Add an ACK payload for the next time around; 1 is the pipe number to acknowledge
      radio.writeAckPayload(1, &message_count, sizeof(message_count));
      ++message_count;  // increment packet counter
    }
  }
}  // check_radio
