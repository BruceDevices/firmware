/*
Last Updated: 30 Mar. 2018
By Anton Grimpelhuber (anton.grimpelhuber@gmail.com)

-----------------------------------------------------------
Semver (http://semver.org/) VERSION HISTORY (newest on top):
(date format: yyyymmdd; ex: 20161022 is 22 Oct. 2016)
------------------------------------------------------------
 - 20180330 - v1.4 - First port to ESP8266 (tested: wemos D1 mini) by Anton Grimpelhuber
 - 20161022 - v1.3 - Semver versioning implemented; various code updates, clarifications, & comment additions,
and changes to fix incompatibilities so it will now compile with latest versions of gcc compiler; also
improved blink indicator routines & added the ability to stop the code-sending sequence once it has begun; by
Gabriel Staples (http://www.ElectricRCAircraftGuy.com)
 - 20101023 - v1.2 - Latest version posted by Ken Shirriff on his website here
(http://www.righto.com/2010/11/improved-arduino-tv-b-gone.html) (direct download link here:
http://arcfn.com/files/arduino-tv-b-gone-1.2.zip)
 - 20101018 - v1.2 - Universality for EU (European Union) & NA (North America) added by Mitch Altman; sleep
mode added by ka1kjz
 - 2010____ - v1.2 - code ported to Arduino; by Ken Shirriff
 - 20090816 - v1.2 - for ATtiny85v, by Mitch Altman & Limor Fried (https://www.adafruit.com/), w/some code by
Kevin Timmerman & Damien Good

TV-B-Gone for Arduino version 1.2, Oct 23 2010
Ported to Arduino by Ken Shirriff
See here: http://www.arcfn.com/2009/12/tv-b-gone-for-arduino.html and here:
http://www.righto.com/2010/11/improved-arduino-tv-b-gone.html (newer)

I added universality for EU (European Union) or NA (North America),
and Sleep mode to Ken's Arduino port
 -- Mitch Altman  18-Oct-2010
Thanks to ka1kjz for the code for adding Sleep
 <http://www.ka1kjz.com/561/adding-sleep-to-tv-b-gone-code/>

The original code is:
TV-B-Gone Firmware version 1.2
 for use with ATtiny85v and v1.2 hardware
 (c) Mitch Altman + Limor Fried 2009
 Last edits, August 16 2009

With some code from:
Kevin Timmerman & Damien Good 7-Dec-07

------------------------------------------------------------
CIRCUIT:
------------------------------------------------------------
-NB: SEE "main.h" TO VERIFY DEFINED PINS TO USE
The hardware for this project uses a wemos D1 mini ESP8266-based board:
 Connect an IR LED to pin 14 / D5 (LED).
 Uses the built-in LED via pin 2.
 Connect a push-button between pin 12 / D6 (TRIGGER) and ground.
 Pin 5 / D1 (REGIONSWITCH) must be left floating for North America, or wire it to ground to have it output
European codes.

More about the wiring is written in the readme.
------------------------------------------------------------
LICENSE:
------------------------------------------------------------
Distributed under Creative Commons 2.5 -- Attribution & Share Alike

*/

#include <Arduino.h>
#include <FS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <SD.h>
#include <globals.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void quickflashLEDx(uint8_t x);
void delay_ten_us(uint16_t us);
void quickflashLED(void);
void StartTvBGone();
void checkIrTxPin();
bool init_ir_tx_mutex();
void lock_ir_tx();
void unlock_ir_tx();
void precise_delay_us(uint32_t us);