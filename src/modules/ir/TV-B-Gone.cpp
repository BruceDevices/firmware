/*
Last Updated: 13/10/2026
By: Ninja-Jr
Optimizations for speed while maintaining 100% compatibility
Added universal power-off codes auto-run after region-specific codes
Added support for raw IR codes with 32-bit timing values
Fixed raw code truncation bug (values > 65535 now split into multi-frame
transmissions instead of being cast to uint16_t)
Made read_bits reentrant via reference-passed state (removed fragile globals)
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
#define MAX_WAIT_TIME 65535 // tens of us (ie: 655.350ms)

extern const IrCode *const NApowerCodes[];
extern const IrCode *const EUpowerCodes[];
extern const IrCode *const UniversalParsedCodes[];
extern const uint8_t num_UniversalParsedCodes;
extern const RawIrCode *const UniversalRawCodes[];
extern const uint8_t num_UniversalRawCodes;
extern const uint8_t num_NAcodes;
extern const uint8_t num_EUcodes;

// Semaphore for thread-safe IR transmission - protects IR LED pin access
static SemaphoreHandle_t ir_tx_mutex = NULL;

// Reentrant bit reader - all state passed by reference, no globals touched
static inline uint8_t read_bits(
    uint8_t count,
    uint8_t &bits,
    uint8_t &bitsleft,
    uint8_t &ptr,
    const volatile IrCode *code
) {
    uint8_t tmp = 0;

    while (count--) {
        if (bitsleft == 0) {
            bits = code->codes[ptr++];
            bitsleft = 8;
        }
        // Shift and OR in one go - faster than separate operations
        tmp = (tmp << 1) | ((bits >> --bitsleft) & 1);
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

// Send parsed protocol codes (16-bit times)
void sendParsedCodeBatch(const IrCode *const *codes, uint8_t count, IRsend &irsend) {
    uint16_t rawData[300];

    for (uint8_t i = 0; i < count; i++) {
        lock_ir_tx();
        const IrCode *pCode = codes[i];

        const uint8_t freq = pCode->timer_val;
        const uint8_t numpairs = pCode->numpairs;
        const uint8_t bitcompression = pCode->bitcompression;

        // Local state - fully reentrant, no global interference
        uint8_t local_bitsleft = 0;
        uint8_t local_bits = 0;
        uint8_t local_ptr = 0;

        for (uint8_t k = 0; k < numpairs; k++) {
            uint16_t ti = read_bits(bitcompression, local_bits, local_bitsleft, local_ptr, pCode) * 2;

            // Bit-shift instead of *10 (faster and deterministic codegen on ESP32)
            uint16_t t0 = pCode->times[ti];
            uint16_t t1 = pCode->times[ti + 1];
            rawData[k * 2]       = (t0 << 3) + (t0 << 1);
            rawData[(k * 2) + 1] = (t1 << 3) + (t1 << 1);
        }

        if (i % 5 == 0) {
            progressHandler(i, count);
        }

        irsend.sendRaw(rawData, (numpairs * 2), freq);
        unlock_ir_tx();
        delay_ten_us(20500);

        if (check(SelPress)) {
            while (check(SelPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
            displayTextLine("Paused");
            while (!check(SelPress)) {
                if (check(EscPress)) { returnToMenu = true; return; }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            while (check(SelPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        if (returnToMenu) break;
    }
    progressHandler(count, count);
}

// Send raw codes (32-bit times - values > 65535 are split into separate frames)
void sendRawCodeBatch(const RawIrCode *const *codes, uint8_t count, IRsend &irsend) {
    uint16_t rawData[300];

    for (uint8_t i = 0; i < count; i++) {
        lock_ir_tx();
        const RawIrCode *pCode = codes[i];

        const uint8_t freq = pCode->timer_val;
        const uint8_t numpairs = pCode->numpairs;

        uint16_t segLen = 0; // number of entries currently buffered in rawData

        for (uint8_t k = 0; k < numpairs; k++) {
            uint32_t on  = pCode->times[k * 2];
            uint32_t off = pCode->times[(k * 2) + 1];

            // A timing > 65535us cannot be represented as a uint16_t pulse.
            // These values represent inter-frame gaps (repeat codes, multi-frame
            // protocols). Flush the current segment, wait the gap, then continue.
            if (on > 65535 || off > 65535) {
                if (segLen > 0) {
                    irsend.sendRaw(rawData, segLen, freq);
                    segLen = 0;
                }
                uint32_t gap = (on > 65535) ? on : off;
                delayMicroseconds(gap);
                continue;
            }

            rawData[segLen++] = (uint16_t)on;
            rawData[segLen++] = (uint16_t)off;
        }

        // Flush any remaining buffered segment
        if (segLen > 0) {
            irsend.sendRaw(rawData, segLen, freq);
        }

        if (i % 5 == 0) {
            progressHandler(i, count);
        }

        unlock_ir_tx();
        delay_ten_us(20500);

        if (check(SelPress)) {
            while (check(SelPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
            displayTextLine("Paused");
            while (!check(SelPress)) {
                if (check(EscPress)) { returnToMenu = true; return; }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            while (check(SelPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        if (returnToMenu) break;
    }
    progressHandler(count, count);
}

void StartTvBGone() {
    if (!init_ir_tx_mutex()) {
        displayRedStripe("Mutex init failed");
        delay(2000);
        return;
    }

    Serial.begin(115200);
    checkIrTxPin();
    IRsend irsend(bruceConfigPins.irTx);
    irsend.begin();
    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);

    // determine region
    options = {
        {"Region NA", [&]() { region = NA; }},
        {"Region EU", [&]() { region = EU; }},
    };
    addOptionToMainMenu();

    loopOptions(options);

    if (!returnToMenu) {
        bool endingEarly = false;

        check(SelPress);

        // Send region-specific codes
        if (region == NA) {
            displayTextLine("Sending NA codes...");
            sendParsedCodeBatch(NApowerCodes, num_NAcodes, irsend);
        } else {
            displayTextLine("Sending EU codes...");
            sendParsedCodeBatch(EUpowerCodes, num_EUcodes, irsend);
        }

        // Send universal parsed codes if user didn't stop
        if (!returnToMenu) {
            displayTextLine("Sending universal parsed codes...");
            sendParsedCodeBatch(UniversalParsedCodes, num_UniversalParsedCodes, irsend);
        }

        // Send universal raw codes if user didn't stop
        if (!returnToMenu) {
            displayTextLine("Sending universal raw codes...");
            sendRawCodeBatch(UniversalRawCodes, num_UniversalRawCodes, irsend);
        }

        // Ensure final progress is shown
        progressHandler(1, 1);

        if (!returnToMenu) {
            displayTextLine("All codes sent!");
            delay_ten_us(MAX_WAIT_TIME);
            delay_ten_us(MAX_WAIT_TIME);
        } else {
            displayRedStripe("User Stopped");
            delay(2000);
        }

        // turnoff LED
        digitalWrite(bruceConfigPins.irTx, LED_OFF);
    }
}
