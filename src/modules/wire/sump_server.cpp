#if !defined(LITE_VERSION)
#include "wire_tools.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/serialcmds.h"
#include "core/settings.h"
#include "core/utils.h"
#include <globals.h>
#include <Arduino.h>

/*********************************************************************
**  SUMP/OLS compatible logic analyzer server (OpenBench Logic Sniffer
**  protocol, as used by sigrok/PulseView and the EUA/ESP32_LogicAnalyzer
**  project). Two transports:
**    - UART pins: connect a USB-UART adapter and select the OLS device:
**        pulseview -D -d ols:conn=/dev/ttyUSB0::serialcomm=921600/8n1
**    - USB CDC: the device's own USB cable (CLI task suspended while
**      active, resumed on exit):
**        pulseview -D -d ols:conn=/dev/ttyACM0::serialcomm=921600/8n1
**
**  Supported: RESET/ID/ARM/METADATA, stage-0 trigger mask+values (channel A),
**  divider (sample rate), read count, flags (ignored). Single-shot capture
**  through the shared RMT logic analyzer core; RLE and delayed triggers are
**  not implemented.
**********************************************************************/

#define SUMP_RESET 0x00
#define SUMP_ARM 0x01
#define SUMP_QUERY 0x02
#define SUMP_SELF_TEST 0x03
#define SUMP_GET_METADATA 0x04
#define SUMP_XON 0x11
#define SUMP_XOFF 0x13

#define SUMP_SET_DIVIDER 0x80
#define SUMP_SET_READ_DELAY_COUNT 0x81
#define SUMP_SET_FLAGS 0x82

#define SUMP_TRIGGER_MASK_CH_A 0xC0
#define SUMP_TRIGGER_VALUES_CH_A 0xC1
#define SUMP_TRIGGER_CONFIG_CH_A 0xC2
#define SUMP_TRIGGER_MASK_CH_B 0xC4
#define SUMP_TRIGGER_VALUES_CH_B 0xC5
#define SUMP_TRIGGER_CONFIG_CH_B 0xC6
#define SUMP_TRIGGER_MASK_CH_C 0xC8
#define SUMP_TRIGGER_VALUES_CH_C 0xC9
#define SUMP_TRIGGER_CONFIG_CH_C 0xCA
#define SUMP_TRIGGER_MASK_CH_D 0xCC
#define SUMP_TRIGGER_VALUES_CH_D 0xCD
#define SUMP_TRIGGER_CONFIG_CH_D 0xCE

// Sample grid capacity: the metadata reports this, hosts never ask for more.
// Samples are streamed on the fly (see wire_la_sump_stream) — no raster buffer,
// so SUMP mode needs no large allocation at all.
#define SUMP_MAX_SAMPLES 16384

static uint32_t sumpBaud = 921600;
static const uint32_t sumpBauds[] = {115200, 230400, 460800, 921600};
static bool sumpUsb = false; // true: SUMP over the USB CDC port (serial CLI suspended)

/*********************************************************************
**  Helpers
**********************************************************************/
static bool sump_read4(Stream &port, uint8_t out[4]) {
    uint32_t deadline = millis() + 100;
    for (int i = 0; i < 4; i++) {
        while (!port.available()) {
            if ((int32_t)(millis() - deadline) >= 0) return false;
            vTaskDelay(1);
        }
        out[i] = (uint8_t)port.read();
        deadline = millis() + 10; // parameter bytes arrive back-to-back
    }
    return true;
}

static uint32_t sump_read4_le(Stream &port) {
    uint8_t b[4];
    if (!sump_read4(port, b)) return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
}

static void sump_write32(Stream &port, uint32_t v) {
    port.write((uint8_t)(v >> 24));
    port.write((uint8_t)(v >> 16));
    port.write((uint8_t)(v >> 8));
    port.write((uint8_t)v);
}

static void sump_send_metadata(Stream &port, uint8_t probes) {
    port.write((uint8_t)0x01); // device name
    port.print("Bruce Logic Analyzer");
    port.write((uint8_t)0x00);
    port.write((uint8_t)0x02); // firmware version
    port.print("1.0");
    port.write((uint8_t)0x00);
    port.write((uint8_t)0x21); // sample memory (samples)
    sump_write32(port, SUMP_MAX_SAMPLES);
    port.write((uint8_t)0x23); // maximum sample rate
    sump_write32(port, 20000000);
    port.write((uint8_t)0x40); // number of probes
    port.write(probes);
    port.write((uint8_t)0x41); // protocol version
    port.write((uint8_t)0x02);
    port.write((uint8_t)0x00); // end of metadata
}

static void sump_draw_idle_screen(uint32_t baud, uint8_t nch) {
    drawMainBorderWithTitle("SUMP Mode");
    padprintln("");
    padprintln("Connect PulseView (OLS driver):");
    if (sumpUsb) {
        padprintln("USB CDC port (CLI suspended)");
    } else {
        padprintln("RX " + String(bruceConfigPins.uart_bus.rx) + " TX " +
                   String(bruceConfigPins.uart_bus.tx) + " @ " + String(baud));
    }
    padprintln("Probes: " + String(nch) + " (" + String(wire_la_resolution() / 1000) + " kHz max)");
    padprintln("");
    displaySuccess("Waiting for host...", false);
}

void sump_server_run(uint32_t baud, bool usb) {
    uint8_t nch = wire_la_channels();
    if (nch == 0) {
        displayError("No channels configured", true);
        return;
    }

    Stream *port;
    if (usb) {
        // The CLI task keeps consuming the USB CDC port; freeze it so SUMP owns
        // the port exclusively. Resumed when this screen exits. The CLI's
        // USBserial wrapper forwards to the core Serial CDC object, which owns
        // the actual RX/TX path.
        vTaskSuspend(serialcmdsTaskHandle);
        port = &Serial;
        sumpUsb = true;
    } else {
        Serial1.end();
        Serial1.setRxBufferSize(2048);
        Serial1.begin(baud, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
        port = &Serial1;
        sumpUsb = false;
    }

    uint32_t divider = 0;
    uint32_t readCount = SUMP_MAX_SAMPLES;
    uint16_t trigMask = 0, trigVals = 0;

    sump_draw_idle_screen(baud, nch);

    while (!check(EscPress)) {
        if (port->available() <= 0) {
            wakeUpScreen();
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }
        uint8_t cmd = (uint8_t)port->read();

        switch (cmd) {
            case SUMP_RESET:
                divider = 0;
                readCount = SUMP_MAX_SAMPLES;
                trigMask = trigVals = 0;
                break;

            case SUMP_QUERY: port->print("1ALS"); break;

            case SUMP_GET_METADATA: sump_send_metadata(*port, nch); break;

            case SUMP_SET_DIVIDER: divider = sump_read4_le(*port); break;

            case SUMP_SET_READ_DELAY_COUNT: {
                uint32_t v = sump_read4_le(*port);
                // Host sends count_low, count_high in the first two bytes (EUA reference:
                // readCount = 4*(((b1<<8)|b0)+1)); v holds them little-endian
                readCount = 4 * ((v & 0xFFFF) + 1);
                if (readCount > SUMP_MAX_SAMPLES) readCount = SUMP_MAX_SAMPLES;
                break; // delayCount (pre-trigger) not supported
            }

            case SUMP_SET_FLAGS:
                sump_read4_le(*port); // RLE / demux / channel groups: not supported
                break;

            case SUMP_TRIGGER_MASK_CH_A: trigMask = sump_read4_le(*port) & 0x0F; break;
            case SUMP_TRIGGER_VALUES_CH_A: trigVals = sump_read4_le(*port) & 0x0F; break;

            // trigger stages B/C/D, trigger configuration, self test: accepted, ignored
            case SUMP_TRIGGER_CONFIG_CH_A:
            case SUMP_TRIGGER_MASK_CH_B:
            case SUMP_TRIGGER_VALUES_CH_B:
            case SUMP_TRIGGER_CONFIG_CH_B:
            case SUMP_TRIGGER_MASK_CH_C:
            case SUMP_TRIGGER_VALUES_CH_C:
            case SUMP_TRIGGER_CONFIG_CH_C:
            case SUMP_TRIGGER_MASK_CH_D:
            case SUMP_TRIGGER_VALUES_CH_D:
            case SUMP_TRIGGER_CONFIG_CH_D:
            case SUMP_SELF_TEST:
                sump_read4_le(*port);
                break;

            case SUMP_ARM: {
                // Requested sample rate is 100 MHz / (divider+1), like the reference
                // OLS device; clamp to the fastest RMT resolution we can hit exactly.
                double rate = 100000000.0 / (divider + 1.0);
                uint32_t resolution = 20000000;
                static const uint32_t rates[] = {100000, 200000, 500000, 1000000, 2000000, 5000000, 10000000, 20000000};
                for (int i = (int)(sizeof(rates) / sizeof(rates[0])) - 1; i >= 0; i--) {
                    if (rates[i] >= rate) resolution = rates[i];
                }

                if (!wire_la_sump_stream(resolution, rate, readCount, trigMask, trigVals, *port)) {
                    displayRedStripe("Capture failed", TFT_WHITE, TFT_RED);
                    break; // failed/aborted: host times out
                }

                port->flush();
                while (port->available()) port->read(); // drop XON/XOFF or stray bytes
                sump_draw_idle_screen(baud, nch); // capture stripe overwrote the info panel
                break;
            }

            default: break; // XON/XOFF and unknown single-byte commands
        }
    }
    check(EscPress);
    if (usb) {
        vTaskResume(serialcmdsTaskHandle);
    } else {
        Serial1.end();
    }
}

/*********************************************************************
**  Function: sump_menu
**  Baudrate selection + server start, shown in the Logic Analyzer menu
**********************************************************************/
void sump_menu() {
    returnToMenu = false;
    while (!returnToMenu) {
        options = {
            {"Port: " + String(sumpUsb ? "USB CDC" : "UART pins"),
             []() { sumpUsb = !sumpUsb; }                                        },
            {"Baud: " + String(sumpBaud), // ignored on USB CDC (baudless)
             []() {
                 for (uint8_t i = 0; i < sizeof(sumpBauds) / sizeof(sumpBauds[0]); i++) {
                     if (sumpBauds[i] == sumpBaud) {
                         sumpBaud = sumpBauds[(i + 1) % (sizeof(sumpBauds) / sizeof(sumpBauds[0]))];
                         return;
                     }
                 }
                 sumpBaud = sumpBauds[0];
             },
             false, nullptr, nullptr, false, !sumpUsb},
            {"Start", []() { sump_server_run(sumpBaud, sumpUsb); }},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "SUMP Mode");
    }
}
#endif
