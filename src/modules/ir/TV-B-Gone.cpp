/*
Last Updated: 30 Mar. 2018
By Anton Grimpelhuber (anton.grimpelhuber@gmail.com)

------------------------------------------------------------
LICENSE:
------------------------------------------------------------
Distributed under Creative Commons 2.5 -- Attribution & Share Alike

*/

#include "TV-B-Gone.h"
#include "WORLD_IR_CODES.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "core/utils.h"
#include "ir_utils.h"
/*
Last Updated: 30 Mar. 2018
By Anton Grimpelhuber (anton.grimpelhuber@gmail.com)
*/

// The TV-B-Gone for Arduino can use either the EU (European Union) or the NA (North America) database of
// POWER CODES EU is for Europe, Middle East, Australia, New Zealand, and some countries in Africa and South
// America NA is for North America, Asia, and the rest of the world not covered by EU

// Two regions!
#define NA 1 // set by a HIGH on REGIONSWITCH pin
#define EU 0 // set by a LOW on REGIONSWITCH pin

// Lets us calculate the size of the NA/EU databases
#define NUM_ELEM(x) (sizeof(x) / sizeof(*(x)));

// set define to 0 to turn off debug output
#define DEBUG 0
#define DEBUGP(x)                                                                                            \
    if (DEBUG == 1) { x; }

// Shortcut to insert single, non-optimized-out nop
#define NOPP __asm__ __volatile__("nop")

// Not used any more on esp8266, so don't bother
// Tweak this if  cessary to change timing
// -for 8MHz Arduinos, a good starting value is 11
// -for 16MHz Arduinos, a good starting value is 25
#define DELAY_CNT 25

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code);
void quickflashLEDx(uint8_t x);
void delay_ten_us(uint16_t us);
void quickflashLED(void);
uint8_t read_bits(uint8_t count);
#define MAX_WAIT_TIME 65535 // tens of us (ie: 655.350ms)
extern const IrCode *const NApowerCodes[];
extern const IrCode *const EUpowerCodes[];
uint8_t num_NAcodes = NUM_ELEM(NApowerCodes);
uint8_t num_EUcodes = NUM_ELEM(EUpowerCodes);
uint8_t bitsleft_r = 0;
uint8_t bits_r = 0;
uint8_t code_ptr;
volatile const IrCode *powerCode;
uint8_t read_bits(uint8_t count) {
    uint8_t i;
    uint8_t tmp = 0;
    for (i = 0; i < count; i++) {
        if (bitsleft_r == 0) {
            bits_r = powerCode->codes[code_ptr++];
            bitsleft_r = 8;
        }
        bitsleft_r--;
        tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count - 1 - i));
    }
    return tmp;
}
uint16_t ontime, offtime;
uint8_t i, num_codes;
uint8_t region;

void delay_ten_us(uint16_t us) {
    uint8_t timer;
    while (us != 0) {
        for (timer = 0; timer <= DELAY_CNT; timer++) {
            NOPP;
            NOPP;
        }
        NOPP;
        us--;
    }
}

void quickflashLED(void) {
#if defined(M5LED)
    digitalWrite(IRLED, M5LED_ON);
    delay_ten_us(3000); // 30 ms ON-time delay
    digitalWrite(IRLED, M5LED_OFF);
#endif
}

void quickflashLEDx(uint8_t x) {
    quickflashLED();
    while (--x) {
        delay_ten_us(25000); // 250 ms OFF-time delay between flashes
        quickflashLED();
    }
}

void checkIrTxPin() {
    const std::vector<std::pair<String, int>> pins = IR_TX_PINS;
    int count = 0;
    for (auto pin : pins) {
        if (pin.second == bruceConfigPins.irTx) count++;
    }
    if (count > 0) return;
    else gsetIrTxPin(true);
}

void StartTvBGone() {
    Serial.begin(115200);
#ifdef USE_BOOST
    PPM.enableOTG();
#endif
    checkIrTxPin();
    IRsend irsend(bruceConfigPins.irTx); // Set the GPIO to be used to sending the message.
    irsend.begin();
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);

    // determine region
    options = {
        {"Region NA", [&]() { region = NA; }},
        {"Region EU", [&]() { region = EU; }},
    };
    addOptionToMainMenu();

    loopOptions(options);
    uint16_t rawData[300];

    if (!returnToMenu) {
        if (region) num_codes = num_NAcodes;
        else num_codes = num_EUcodes;

        bool endingEarly = false; // will be set to true if the user presses the button during code-sending

        check(SelPress);
        for (i = 0; i < num_codes; i++) {
            if (region == NA) powerCode = NApowerCodes[i];
            else powerCode = EUpowerCodes[i];

            const uint8_t freq = powerCode->timer_val;
            const uint8_t numpairs = powerCode->numpairs;
            const uint8_t bitcompression = powerCode->bitcompression;

            // For EACH pair in this code....
            code_ptr = 0;
            for (uint8_t k = 0; k < numpairs; k++) {
                uint16_t ti;
                ti = (read_bits(bitcompression)) * 2;
                offtime = powerCode->times[ti];    // read word 1 - ontime
                ontime = powerCode->times[ti + 1]; // read word 2 - offtime

                rawData[k * 2] = offtime * 10;
                rawData[(k * 2) + 1] = ontime * 10;
            }
            progressHandler(i, num_codes);
            irsend.sendRaw(rawData, (numpairs * 2), freq);
            bitsleft_r = 0;
            delay_ten_us(20500);

            // if user is pushing (holding down) TRIGGER button, stop transmission early
            if (check(SelPress)) // Pause TV-B-Gone
            {
                while (check(SelPress)) yield();
                displayTextLine("Paused");

                while (!check(SelPress)) { // If Presses Select again, continues
                    if (check(EscPress)) {
                        endingEarly = true;
                        break;
                    }
                }
                while (check(SelPress)) { yield(); }
                if (endingEarly) break; // Cancels  TV-B-Gone
                displayTextLine("Running, Wait");
            }

        } // end of POWER code for loop

        if (endingEarly == false) {
            displayTextLine("All codes sent!");
            // pause for ~1.3 sec, then flash the visible LED 8 times to indicate that we're done
            delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
            delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
        } else {
            displayRedStripe("User Stopped");
            delay(2000);
        }

        // turnoff LED
        digitalWrite(bruceConfigPins.irTx, LED_OFF);

#ifdef USE_BOOST

        /// DISABLE 5V OUTPUT
        PPM.disableOTG();
#endif
    }
} // end of sendAllCodes
