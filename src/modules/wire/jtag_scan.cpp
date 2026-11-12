#if !defined(LITE_VERSION)
#include "wire_tools.h"

#include "core/display.h"
#include <globals.h>
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "core/utils.h"
#include <Arduino.h>

// Session pin assignment for the UI (serial commands pass pins explicitly)
static int8_t jtagTck = -1, jtagTms = -1, jtagTdi = -1, jtagTdo = -1;

// Bit-bang pacing (~250 kHz TCK): far below any target's tolerance, robust on jumper wires
#define JTAG_TCK_HALF_US 2

/*********************************************************************
**  Low-level JTAG bit-bang (IEEE 1149.1 TAP)
**********************************************************************/
static inline void jtagTckPulse(int8_t tck) {
    digitalWrite(tck, LOW);
    delayMicroseconds(JTAG_TCK_HALF_US);
    digitalWrite(tck, HIGH);
    delayMicroseconds(JTAG_TCK_HALF_US);
}

// Bring the TAP to Test-Logic-Reset (5 TCK with TMS high), then to Run-Test/Idle
static void jtagTapReset(int8_t tck, int8_t tms) {
    digitalWrite(tms, HIGH);
    for (int i = 0; i < 5; i++) jtagTckPulse(tck);
    digitalWrite(tms, LOW);
    jtagTckPulse(tck);
}

// Reset -> Shift-DR, shift 32 bits LSB-first, then exit to Run-Test/Idle
static uint32_t jtagShiftDr32(int8_t tck, int8_t tms, int8_t tdi, int8_t tdo) {
    digitalWrite(tms, HIGH); jtagTckPulse(tck); // Select-DR-Scan
    digitalWrite(tms, LOW);  jtagTckPulse(tck); // Capture-DR
    jtagTckPulse(tck);                          // Shift-DR
    digitalWrite(tms, LOW);

    uint32_t idcode = 0;
    for (int bit = 0; bit < 32; bit++) {
        bool exitShift = (bit == 31);
        digitalWrite(tms, exitShift ? HIGH : LOW);
        digitalWrite(tdi, LOW);
        digitalWrite(tck, LOW);          // TDO is valid while TCK is low
        delayMicroseconds(JTAG_TCK_HALF_US);
        if (digitalRead(tdo)) idcode |= (1UL << bit);
        digitalWrite(tck, HIGH);
        delayMicroseconds(JTAG_TCK_HALF_US);
    }

    digitalWrite(tms, HIGH); jtagTckPulse(tck); // Update-DR
    digitalWrite(tms, LOW);  jtagTckPulse(tck); // Run-Test/Idle
    return idcode;
}

/*********************************************************************
**  Serial-command primitive (wire jtag idcode), see wire_tools.h
**********************************************************************/
String wire_jtag_read_idcode(int8_t tck, int8_t tms, int8_t tdi, int8_t tdo) {
    if (tck < 0 || tms < 0 || tdi < 0 || tdo < 0) return "ERR: assign TCK/TMS/TDI/TDO first";
    if (tck == tms || tck == tdi || tck == tdo || tms == tdi || tms == tdo || tdi == tdo)
        return "ERR: pins must be distinct";

    pinMode(tck, OUTPUT);
    pinMode(tms, OUTPUT);
    pinMode(tdi, OUTPUT);
    pinMode(tdo, INPUT_PULLUP); // pull-up keeps an unconnected TDO from floating
    digitalWrite(tck, LOW);
    digitalWrite(tms, LOW);
    digitalWrite(tdi, LOW);

    jtagTapReset(tck, tms);
    uint32_t idcode = jtagShiftDr32(tck, tms, tdi, tdo);
    // On a real target the IDCODE register is selected by default after TAP reset;
    // a second read costs nothing and filters glitches on long wires.
    if (idcode == 0 || idcode == 0xFFFFFFFF || (idcode & 1) == 0) {
        jtagTapReset(tck, tms);
        idcode = jtagShiftDr32(tck, tms, tdi, tdo);
    }

    pinMode(tck, INPUT);
    pinMode(tms, INPUT);
    pinMode(tdi, INPUT);

    if (idcode == 0 || idcode == 0xFFFFFFFF || (idcode & 1) == 0) return "ERR: no JTAG target found";

    uint8_t version = (idcode >> 28) & 0xF;
    uint16_t part = (idcode >> 12) & 0xFFFF;
    uint16_t manufacturer = (idcode >> 1) & 0x7FF;
    uint8_t jepContinuation = (manufacturer >> 7) & 0xF;
    uint8_t jepIdentity = manufacturer & 0x7F;

    char out[96];
    snprintf(out, sizeof(out), "OK 0x%08lX (ver=%u part=0x%04X mfr:bank=%u id=0x%02X)",
             (unsigned long)idcode, version, part, jepContinuation + 1, jepIdentity);
    return String(out);
}

/*********************************************************************
**  Function: jtag_pick_pin
**  GPIO list picker, same interaction as Config > UART Pins
**********************************************************************/
static int8_t jtag_pick_pin(const char *title, int8_t current) {
    options.clear();
    int8_t picked = current;
    for (int i = -1; i <= GPIO_NUM_MAX; i++) {
        options.push_back({String(i), [i, &picked]() { picked = (int8_t)i; }, i == current});
    }
    int index = (current < 0) ? 0 : current + 1;
    loopOptions(options, MENU_TYPE_SUBMENU, title, index);
    return picked;
}

/*********************************************************************
**  Function: jtag_scan_setup
**********************************************************************/
void jtag_scan_setup() {
    returnToMenu = false;
    while (!returnToMenu) {
        options = {
            {String("TCK: ") + int(jtagTck),  []() { jtagTck = jtag_pick_pin("TCK Pin", jtagTck); }  },
            {String("TMS: ") + int(jtagTms),  []() { jtagTms = jtag_pick_pin("TMS Pin", jtagTms); }  },
            {String("TDI: ") + int(jtagTdi),  []() { jtagTdi = jtag_pick_pin("TDI Pin", jtagTdi); }  },
            {String("TDO: ") + int(jtagTdo),  []() { jtagTdo = jtag_pick_pin("TDO Pin", jtagTdo); }  },
            {String("Scan IDCODE"),           []() {
                 displayRedStripe("Scanning..");
                 String result = wire_jtag_read_idcode(jtagTck, jtagTms, jtagTdi, jtagTdo);
                 if (!result.startsWith("OK")) {
                     displayError(result.startsWith("ERR") ? result.substring(5) : result, true);
                     return;
                 }
                 drawMainBorderWithTitle("JTAG IDCODE");
                 padprintln("");
                 padprintln("IDCODE: " + result.substring(3));
                 padprintln("");
                 padprintln("TCK:" + String(jtagTck) + " TMS:" + String(jtagTms));
                 padprintln("TDI:" + String(jtagTdi) + " TDO:" + String(jtagTdo));
                 padprintln("");
                 displaySuccess("Target found", false);
                 while (!check(EscPress) && !check(SelPress)) { wakeUpScreen(); vTaskDelay(pdMS_TO_TICKS(20)); }
                 check(EscPress); check(SelPress);
             }},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "JTAG Scanner");
    }
}
#endif
