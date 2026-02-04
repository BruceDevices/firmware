/*
Last Updated: 24/01/2026
By: Ninja-Jr
Optimizations for speed while maintaining 100% compatibility

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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

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

// Semaphore for thread-safe IR transmission - protects IR LED pin access
static SemaphoreHandle_t ir_tx_mutex = NULL;

// Optimized bit reading - combines shift and bit test in single operation
uint8_t read_bits(uint8_t count) {
    uint8_t tmp = 0;

    while (count--) {
        if (bitsleft_r == 0) {
            bits_r = powerCode->codes[code_ptr++];
            bitsleft_r = 8;
        }
        // Shift and OR in one go - faster than separate operations
        tmp = (tmp << 1) | ((bits_r >> --bitsleft_r) & 1);
    }
    return tmp;
}

uint16_t ontime, offtime;
uint8_t i, num_codes;
uint8_t region;

// Microsecond delay using NOPs - keeps timing tight without blocking RTOS
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

// Mutex setup - one-time initialization
bool init_ir_tx_mutex() {
    if (ir_tx_mutex == NULL) {
        ir_tx_mutex = xSemaphoreCreateMutex();
        if (ir_tx_mutex == NULL) {
            return false;
        }
    }
    return true;
}

void lock_ir_tx() {
    if (ir_tx_mutex != NULL) {
        xSemaphoreTake(ir_tx_mutex, portMAX_DELAY);
    }
}

void unlock_ir_tx() {
    if (ir_tx_mutex != NULL) {
        xSemaphoreGive(ir_tx_mutex);
    }
}

void StartTvBGone() {
    if (!init_ir_tx_mutex()) {
        displayRedStripe("Mutex init failed");
        delay(2000);
        return;
    }

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
            // Lock only during IR transmission - allows button checks to run while waiting
            lock_ir_tx();
            
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

                // Direct calculation - skips temp variable for speed
                rawData[k * 2] = powerCode->times[ti] * 10;        // offtime * 10
                rawData[(k * 2) + 1] = powerCode->times[ti + 1] * 10; // ontime * 10
            }

            // Update progress every 5 codes instead of every code - reduces UI overhead
            if (i % 5 == 0) {
                progressHandler(i, num_codes);
            }

            irsend.sendRaw(rawData, (numpairs * 2), freq);
            unlock_ir_tx();  // Release mutex immediately so other tasks can run
            bitsleft_r = 0;
            
            // Wait 205ms between codes using NOP delays (keeps timing precise)
            delay_ten_us(20500);

            // Check for user input - this can run freely since mutex is unlocked
            if (check(SelPress)) // Pause TV-B-Gone
            {
                while (check(SelPress)) {
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                displayTextLine("Paused");

                while (!check(SelPress)) { // If Presses Select again, continues
                    if (check(EscPress)) {
                        endingEarly = true;
                        break;
                    }
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                while (check(SelPress)) {
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                if (endingEarly) break; // Cancels  TV-B-Gone
                displayTextLine("Running, Wait");
            }

        } // end of POWER code for loop

        // Ensure final progress is shown
        progressHandler(num_codes, num_codes);

        if (endingEarly == false) {
            displayTextLine("All codes sent!");
            // pause for ~1.3 sec using efficient NOP delays
            delay_ten_us(MAX_WAIT_TIME);
            delay_ten_us(MAX_WAIT_TIME);
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
