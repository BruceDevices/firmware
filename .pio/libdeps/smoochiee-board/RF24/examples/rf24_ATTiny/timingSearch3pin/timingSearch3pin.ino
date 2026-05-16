/**
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * written by tong67 (https://github.com/tong67)
 * edited by 2bndy5 (http://github.com/2bndy5) for compatibility with SpenceKonde's ATTinyCore
 */

/*
 * This sketch can determine the best settle time values to use for
 * macros, defined as RF24_CSN_SETTLE_HIGH_DELAY and RF24_CSN_SETTLE_LOW_DELAY,
 * in RF24::csn().
 * The settle time values used here are 100/20. However, these values depend
 * on the actual used RC combination and voltage drop by LED. The
 * intermediate results are written to TX (PB3, pin 2 -- using Serial).
 *
 * For schematic details, see introductory comment block in the
 * examples/rf24_ATTiny/rf24ping85/rf24ping85.ino sketch.
 */

#include <stdio.h>
#include <SPI.h>
#include <Arduino.h>
#include <nRF24L01.h>


#if defined(ARDUINO) && !defined(__arm__)
#if defined(__AVR_ATtinyX5__) || defined(__AVR_ATtinyX4__)
#define RF24_TINY
#endif
#endif

/****************************************************************************/

#if defined(RF24_TINY)

// when Attiny84 or Attiny85 is detected
#define CE_PIN 3  /** "Chip Enable" pin, activates the RX or TX role */
#define CSN_PIN 3 /** SPI Chip Select Not */

#else
// when not running on an ATTiny84 or ATTiny85
#define CE_PIN 7  /** "Chip Enable" pin, activates the RX or TX role */
#define CSN_PIN 8 /** SPI Chip Select Not */

#endif

#define MAX_HIGH 100
#define MAX_LOW 100
#define MINIMAL 8

// Use these adjustable variables to test for best configuration to be used on
// the ATTiny chips. These variables are defined as macros in the library's
// RF24/utility/ATTiny/RF24_arch_config.h file. To change them, simply define
// the corresponding macro(s) before #include <RF24> in your sketch.
uint8_t csnHighSettle = MAX_HIGH;  // defined as RF24_CSN_SETTLE_HIGH_DELAY
uint8_t csnLowSettle = MAX_LOW;    // defined as RF24_CSN_SETTLE_LOW_DELAY

/****************************************************************************/
void ce(bool level) {
  if (CE_PIN != CSN_PIN) digitalWrite(CE_PIN, level);
}

/****************************************************************************/
void csn(bool mode) {
  if (CE_PIN != CSN_PIN) {
    digitalWrite(CSN_PIN, mode);
  } else {
    // digitalWrite(SCK, mode);
    if (mode == HIGH) {
      PORTB |= (1 << PINB2);             // SCK->CSN HIGH
      delayMicroseconds(csnHighSettle);  // allow csn to settle
    } else {
      PORTB &= ~(1 << PINB2);           // SCK->CSN LOW
      delayMicroseconds(csnLowSettle);  // allow csn to settle
    }
  }
}

/****************************************************************************/
uint8_t read_register(uint8_t reg) {
  csn(LOW);
  SPI.transfer(reg);
  uint8_t result = SPI.transfer(0xff);
  csn(HIGH);
  return result;
}

/****************************************************************************/
void write_register(uint8_t reg, uint8_t value) {
  csn(LOW);
  SPI.transfer(W_REGISTER | reg);
  SPI.transfer(value);
  csn(HIGH);
}

/****************************************************************************/
void setup(void) {

#ifndef __AVR_ATtinyX313__
  // not enough memory on ATTiny4313 or ATTint2313(a) to use Serial I/O for this sketch

  // start serial port and SPI
  Serial.begin(115200);
  SPI.begin();
  // configure CE and CSN as output when used
  pinMode(CE_PIN, OUTPUT);
  if (CSN_PIN != CE_PIN)
    pinMode(CSN_PIN, OUTPUT);

  // csn is used in SPI transfers. Set to LOW at start and HIGH after transfer. Set to HIGH to reflect no transfer active
  // SPI command are accepted in Power Down state.
  // CE pin represent PRX (LOW) or PTX (HIGH) mode apart from register settings. Start in PRX mode.
  ce(LOW);
  csn(HIGH);

  // nRF24L01 goes from to Power Down state 100ms after Power on Reset ( Vdd > 1.9V) or when PWR_UP is 0 in config register
  // Goto Power Down state (Powerup or force) and set in transmit mode
  write_register(NRF_CONFIG, read_register(NRF_CONFIG) & ~_BV(PWR_UP) & ~_BV(PRIM_RX));
  delay(100);

  // Goto Standby-I
  // Technically we require 4.5ms Tpd2stby+ 14us as a worst case. We'll just call it 5ms for good measure.
  // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
  write_register(NRF_CONFIG, read_register(NRF_CONFIG) | _BV(PWR_UP));
  delay(5);

  // Goto Standby-II
  ce(HIGH);
  Serial.print("Scanning for optimal setting time for csn");


  /************************** Main program *********************************/

  uint8_t result;  // used to compare read/write results with read/write cmds
  bool success = true;
  uint8_t bottom_success;
  bool bottom_found;
  uint8_t value[] = { 5, 10 };
  uint8_t limit[] = { MAX_HIGH, MAX_LOW };
  uint8_t advice[] = { MAX_HIGH, MAX_LOW };

  // check max values give correct behavior
  for (uint8_t k = 0; k < 2; k++) {
    bottom_found = false;
    bottom_success = 0;
    while (bottom_success < 255) {
      csnHighSettle = limit[0];
      csnLowSettle = limit[1];
      // check current values
      uint8_t i = 0;
      while (i < 255 && success) {
        for (uint8_t j = 0; j < 2; j++) {
          write_register(EN_AA, value[j]);
          result = read_register(EN_AA);
          if (value[j] != result) {
            success = false;
          }
        }
        i++;
      }
      // process result of current values
      if (!success) {
        Serial.print("Settle Not OK. csnHigh=");
        Serial.print(limit[0], DEC);
        Serial.print(" csnLow=");
        Serial.println(limit[1], DEC);
        limit[k]++;
        bottom_found = true;
        bottom_success = 0;
        success = true;
      } else {
        Serial.print("Settle OK. csnHigh=");
        Serial.print(limit[0], DEC);
        Serial.print(" csnLow=");
        Serial.println(limit[1], DEC);
        if (!bottom_found) {
          limit[k]--;
          if (limit[k] == MINIMAL) {
            bottom_found = true;
            bottom_success = 0;
            success = true;
          }
        } else {
          bottom_success++;
        }
      }
    }  // while (bottom_success < 255)
    Serial.print("Settle value found for ");
    if (k == 0) {
      Serial.print("csnHigh: ");
    } else {
      Serial.print("csnLow: ");
    }
    Serial.println(limit[k], DEC);
    advice[k] = limit[k] + (limit[k] / 10);
    limit[k] = 100;
  }  // for (uint8_t k = 0; k < 2; k++)
  Serial.print("Advised Settle times are: csnHigh=");
  Serial.print(advice[0], DEC);
  Serial.print(" csnLow=");
  Serial.println(advice[1], DEC);

#endif  // not defined __AVR_ATtinyX313__
}


void loop(void) {}  // this program runs only once, thus it resides in setup()
