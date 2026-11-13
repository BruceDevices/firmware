#include "hc11_modem.h"

// Arduino.h pulls in soc_caps.h, which defines SOC_USB_OTG_SUPPORTED on
// chips with a native USB OTG controller - the check below depends on it.
#include <Arduino.h>

#if defined(SOC_USB_OTG_SUPPORTED)

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/serialcmds.h"
#include "globals.h"
#include "hc11_at.h"
#include "hc11_radio.h"
#include <USB.h>

/*
 * HC-11 USB modem app (real RF over CC1101, see hc11_radio.*).
 *
 * USB ownership model: the firmware console (serialcmds task) already has the
 * USB CDC port up and running. This app does NOT re-init the USB stack
 * (calling USB.begin() while the CDC session is live wedges the OTG
 * peripheral - bench-confirmed on T-Embed CC1101: the port disappears and
 * survives a soft reset). Instead we suspend the console task (via the
 * serialcmdsTaskHandle exported by core/serialcmds.h) and read the same CDC
 * port ourselves; on exit the console is resumed on the still-live port, so
 * the host connection never drops.
 *
 * All mutable state is static: USB initialization can corrupt stack
 * variables (see src/modules/others/clicker.cpp warning).
 */

// Take the USB CDC port from the firmware console task / give it back.
static void hc11SuspendConsole() {
    if (serialcmdsTaskHandle != nullptr) vTaskSuspend(serialcmdsTaskHandle);
}

static void hc11ResumeConsole() {
    if (serialcmdsTaskHandle != nullptr) vTaskResume(serialcmdsTaskHandle);
}

static Hc11Session hc11Session;
static String hc11LineBuf;
static volatile bool hc11CdcConnected = false;
static bool hc11UsbActive = false;

// Max length of one AT command line; longer input is treated as garbage
// and dropped (the real module has a small UART buffer too).
#define HC11_MAX_LINE 128

// ------------------------------------------------------------------
// USB CDC events (massStorage pattern)
// ------------------------------------------------------------------

static void hc11UsbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_data;
    if (event_base != ARDUINO_USB_EVENTS) return;
    if (event_id == ARDUINO_USB_STARTED_EVENT) hc11CdcConnected = true;
    else if (event_id == ARDUINO_USB_STOPPED_EVENT) hc11CdcConnected = false;
}

// Is a USB host attached? On HWCDC boards (ARDUINO_USB_MODE=1, e.g. T-Embed,
// Cardputer) Serial is the hardware USB-Serial/JTAG peripheral and the TinyUSB
// mount/unmount events above never fire -- poll the SOF-based connection
// monitor instead (debounced: the monitor is known to flap for a few ms even
// on a healthy link). On TinyUSB CDC boards the event flag is authoritative.
static bool hc11HostAttached() {
#if defined(ARDUINO_USB_CDC_ON_BOOT) && defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE == 1
    static unsigned long lastPluggedMs = 0;
    if (Serial.isPlugged()) {
        lastPluggedMs = millis();
        return true;
    }
    return millis() - lastPluggedMs < 500;
#else
    return hc11CdcConnected;
#endif
}

// ------------------------------------------------------------------
// Host I/O
// ------------------------------------------------------------------

static void hc11SendToHost(const String &s) {
    if (s.length() == 0) return;
    Serial.write((const uint8_t *)s.c_str(), s.length());
    hc11Session.radioToHostBytes += s.length();
    hc11Session.logBytes((const uint8_t *)s.c_str(), s.length(), false);
}

static void hc11FeedCommandByte(uint8_t b) {
    if (b == '\n') {
        String line = hc11LineBuf;
        hc11LineBuf = "";
        Hc11Module before = hc11Session.module;
        String resp = hc11HandleAtLine(hc11Session, line);
        // Radio-relevant parameter changes apply to the radio immediately,
        // without an app restart. The AT layer itself stays untouched.
        const Hc11Module &m = hc11Session.module;
        if (m.channel != before.channel || m.address != before.address || m.txPower != before.txPower ||
            m.mode != before.mode)
            hc11RadioApplyConfig(m.channel, m.address, m.txPower, m.mode);
        if (resp.length() > 0) hc11SendToHost(resp + "\r\n");
    } else if (b != '\r') {
        if (hc11LineBuf.length() < HC11_MAX_LINE) hc11LineBuf += (char)b;
        else hc11LineBuf = ""; // overflow: drop the line, like a small UART buffer would
    }
}

// Payload of a valid HC-11 packet received over the air: hand it to the
// host port (transparent mode only - in AT command mode RF data is not
// delivered as payload, like the real module).
// Timestamp of the last delivered byte: the passthrough loop defers screen
// redraws while a burst is still being received (a redraw blocks the radio
// drain and the RX FIFO overflows after ~10 back-to-back packets).
static unsigned long hc11LastRxMs = 0;

static void hc11RadioRxDelivery(const uint8_t *data, size_t len, void *ctx) {
    (void)ctx;
    if (hc11Session.commMode != HC11_MODE_TRANSPARENT) return;
    Serial.write(data, len);
    hc11Session.radioToHostBytes += len;
    hc11Session.logBytes(data, len, false);
    hc11LastRxMs = millis();
}

// Must be called from every app UI loop so host data is drained even while
// the user is navigating the on-device screens (edge case: no silent loss).
// Also drives the radio state machine (RX packets, TX bursts, live config).
static void hc11ProcessUsb() {
    if (!hc11UsbActive) return;
    hc11RadioPoll();
    uint8_t buf[64];
    while (Serial.available() > 0) {
        size_t n = Serial.read(buf, sizeof(buf));
        if (n == 0) break;
        if (hc11Session.commMode == HC11_MODE_TRANSPARENT) {
            // Transparent mode: data goes to the radio channel and is
            // transmitted over the air in HC-11 format. The radio increments
            // the TX counter after transmission; the log records bytes from
            // the host.
            hc11RadioFeed(buf, n);
            hc11Session.logBytes(buf, n, true);
        } else {
            for (size_t i = 0; i < n; i++) hc11FeedCommandByte(buf[i]);
        }
    }
}

// ------------------------------------------------------------------
// Mode switching - analog of the HC-11 "SET" pin
// ------------------------------------------------------------------

static void hc11SetCommMode(Hc11CommMode newMode) {
    if (newMode == hc11Session.commMode) return;
    hc11Session.commMode = newMode;
    hc11LineBuf = "";
    if (newMode == HC11_MODE_COMMAND) {
        // Datasheet: after leaving the AT command mode the module is in a
        // reset state; wait >= 200 ms before entering the mode again.
        delay(200);
    }
}

// ------------------------------------------------------------------
// Screens
// ------------------------------------------------------------------

static String hc11ModeLabel() {
    return hc11Session.commMode == HC11_MODE_COMMAND ? "AT COMMAND" : "TRANSPARENT";
}

static void hc11DrawStatusScreen() {
    drawMainBorderWithTitle("HC-11 Modem");
    padprintln("");
    padprintln("Mode:  " + hc11ModeLabel());
    padprintln(
        "Ch: " + hc11ChannelString(hc11Session.module.channel) +
        "  Addr: " + hc11AddressString(hc11Session.module.address)
    );
    padprintln(
        "Pwr: " + hc11PowerString(hc11Session.module.txPower) +
        "  FU: " + hc11ModeString(hc11Session.module.mode)
    );
    padprintln("Baud: " + String(hc11Session.module.baud));
    padprintln("USB:  " + String(hc11HostAttached() ? "connected" : "disconnected"));
    // Radio link status: RX/TX, RSSI of the last packet, overflow
    Hc11RadioStatus rs = hc11RadioGetStatus();
    String radio = rs.state == HC11_RADIO_RX ? "RX" : rs.state == HC11_RADIO_TX_CHAIN ? "TX" : "OFF";
    padprintln("Radio: " + radio + "  RSSI: " + String(rs.lastRssiDbm, 1) + " dBm");
    padprintln("");
    // TX counts bytes actually sent over the air, incremented by the radio
    // on completed packets.
    padprintln("Host->Radio: " + String(hc11Session.hostToRadioBytes) + " B");
    padprintln("Radio->Host: " + String(hc11Session.radioToHostBytes) + " B");
    if (rs.overflow || rs.droppedBytes > 0)
        padprintln(
            "Dropped: " + String(rs.droppedBytes) + " B" + String(rs.overflow ? "  (TX overflow!)" : "")
        );
    padprintln("");
    padprintln("[OK] AT/Transparent  [<>] Log");
    padprintln("[ESC] Menu");
}

static void hc11DrawByteLogScreen() {
    drawMainBorderWithTitle("HC-11 Traffic Log");
    padprintln("");
    size_t count = hc11Session.byteLogLen;
    size_t show = count < 60 ? count : 60; // last bytes, newest last
    String line;
    size_t perLine = 0;
    for (size_t i = show; i > 0; i--) {
        size_t idx = (hc11Session.byteLogStart + hc11Session.byteLogLen - i) % HC11_BYTE_LOG_SIZE;
        char buf[8];
        snprintf(
            buf,
            sizeof(buf),
            "%s%02X ",
            hc11Session.byteLogFromHost[idx] ? ">" : "<",
            hc11Session.byteLog[idx]
        );
        line += buf;
        if (++perLine >= 6) {
            padprintln(line);
            line = "";
            perLine = 0;
        }
    }
    if (perLine > 0) padprintln(line);
    if (show == 0) padprintln("(no traffic yet)");
    padprintln("");
    padprintln("'>': host->radio  '<': radio->host");
    padprintln("[<>] Status  [ESC] Menu");
}

static void hc11DrawAboutScreen() {
    drawMainBorderWithTitle("About");
    padprintln("");
    padprintln("HC-11 Modem (real RF)");
    padprintln("HC-11 V1.9 over CC1101, 433 MHz");
    padprintln("");
    padprintln("Compatible with HC-11");
    padprintln("radio modules and peers.");
    padprintln("");
    padprintln("AT protocol per the official");
    padprintln("HC-11 user manual (hc01.com).");
    padprintln("");
    padprintln("Bruce firmware");
    padprintln("");
    padprintln("[any key] Back");
}

// ------------------------------------------------------------------
// Configuration screen
// ------------------------------------------------------------------

enum Hc11ConfigItem { CFG_CHANNEL = 0, CFG_ADDRESS, CFG_POWER, CFG_MODE, CFG_BAUD, CFG_COUNT };

static const char *hc11ConfigItemName(Hc11ConfigItem item) {
    switch (item) {
        case CFG_CHANNEL: return "Channel";
        case CFG_ADDRESS: return "Address";
        case CFG_POWER: return "TX Power";
        case CFG_MODE: return "Mode";
        case CFG_BAUD: return "Baud";
        default: return "?";
    }
}

static String hc11ConfigItemValue(Hc11ConfigItem item) {
    switch (item) {
        case CFG_CHANNEL: return hc11ChannelString(hc11Session.module.channel);
        case CFG_ADDRESS: return hc11AddressString(hc11Session.module.address);
        case CFG_POWER: return hc11PowerString(hc11Session.module.txPower);
        case CFG_MODE: return hc11ModeString(hc11Session.module.mode);
        case CFG_BAUD: return String(hc11Session.module.baud);
        default: return "?";
    }
}

static void hc11ConfigChange(Hc11ConfigItem item, int dir) {
    Hc11Module &m = hc11Session.module;
    switch (item) {
        case CFG_CHANNEL: {
            int v = (int)m.channel + dir;
            if (v < 1) v = 127;
            if (v > 127) v = 1;
            m.channel = (uint16_t)v;
            break;
        }
        case CFG_ADDRESS: {
            int v = (int)m.address + dir;
            if (v < 0) v = 255;
            if (v > 255) v = 0;
            m.address = (uint16_t)v;
            break;
        }
        case CFG_POWER: {
            int v = (int)m.txPower + dir;
            if (v < 1) v = 8;
            if (v > 8) v = 1;
            m.txPower = (uint8_t)v;
            break;
        }
        case CFG_MODE: {
            int v = (int)m.mode + dir;
            if (v < 1) v = 4;
            if (v > 4) v = 1;
            m.mode = (uint8_t)v;
            break;
        }
        case CFG_BAUD: {
            static const uint32_t bauds[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
            const int n = sizeof(bauds) / sizeof(bauds[0]);
            int cur = 0;
            for (int i = 0; i < n; i++) {
                if (bauds[i] == m.baud) {
                    cur = i;
                    break;
                }
            }
            cur = (cur + dir + n) % n;
            m.baud = bauds[cur];
            break;
        }
        default: break;
    }
    // Apply radio-relevant changes to the radio immediately;
    // baud only affects the host-side UART emulation, not the radio.
    hc11RadioApplyConfig(m.channel, m.address, m.txPower, m.mode);
}

static void hc11ConfigScreen() {
    int item = CFG_CHANNEL;
    bool editing = false;
    bool redraw = true;
    while (true) {
        hc11ProcessUsb(); // keep draining the host while configuring
        if (check(EscPress)) {
            if (editing) {
                editing = false;
                redraw = true;
            } else break;
        }
#ifdef HAS_ENCODER
        // Same control scheme as the firmware's own option menus on encoder
        // boards (e.g. loopOptions in display.cpp): positive steps move the
        // selection up, negative steps move it down; while editing, the wheel
        // changes the value the same way. Encoder rotation also latches
        // Prev/Next/Up/Down flags -- consume them so they aren't double-applied.
        int32_t steps = drainRotarySteps();
        if (steps != 0) {
            check(PrevPress);
            check(NextPress);
            check(UpPress);
            check(DownPress);
            int dir = steps > 0 ? -1 : 1; // firmware convention: positive = up
            for (int32_t n = steps > 0 ? steps : -steps; n > 0; n--) {
                if (editing) hc11ConfigChange((Hc11ConfigItem)item, dir);
                else item = (item + CFG_COUNT + dir) % CFG_COUNT;
            }
            redraw = true;
        }
#else
        if (editing) {
            if (check(PrevPress)) {
                hc11ConfigChange((Hc11ConfigItem)item, -1);
                redraw = true;
            }
            if (check(NextPress)) {
                hc11ConfigChange((Hc11ConfigItem)item, +1);
                redraw = true;
            }
        } else {
            if (check(UpPress)) {
                item = (item + CFG_COUNT - 1) % CFG_COUNT;
                redraw = true;
            }
            if (check(DownPress)) {
                item = (item + 1) % CFG_COUNT;
                redraw = true;
            }
        }
#endif
        if (check(SelPress)) {
            editing = !editing;
            redraw = true;
        }
        if (redraw) {
            drawMainBorderWithTitle("HC-11 Configuration");
            padprintln("");
            for (int i = 0; i < CFG_COUNT; i++) {
                String prefix = (i == item) ? (editing ? "# " : "> ") : "  ";
                padprintln(
                    prefix + hc11ConfigItemName((Hc11ConfigItem)i) + ": " +
                    hc11ConfigItemValue((Hc11ConfigItem)i)
                );
            }
            padprintln("");
#ifdef HAS_ENCODER
            padprintln("[Wheel] Select/Change [OK] Edit");
#else
            padprintln("[v^] Select  [<>] Change  [OK] Edit");
#endif
            padprintln("[ESC] Back");
            redraw = false;
        }
        delay(10);
    }
}

// ------------------------------------------------------------------
// USB passthrough loop (traffic screens + SET-pin toggle)
// ------------------------------------------------------------------

static void hc11PassthroughLoop() {
    int screen = 0; // 0 = status/counters, 1 = byte log
    bool redraw = true;
    unsigned long lastDraw = 0;

    while (true) {
        hc11ProcessUsb();

        if (check(EscPress)) break; // back to app submenu

        // OK button: toggle AT command / transparent mode (SET pin analog)
        if (check(SelPress)) {
            hc11SetCommMode(
                hc11Session.commMode == HC11_MODE_COMMAND ? HC11_MODE_TRANSPARENT : HC11_MODE_COMMAND
            );
            redraw = true;
        }

        if (check(NextPress)) {
            screen = 1;
            redraw = true;
        }
        if (check(PrevPress)) {
            screen = 0;
            redraw = true;
        }

        unsigned long now = millis();
        if (redraw || now - lastDraw >= 250) { // ~4 Hz refresh
            // A full-screen redraw takes a long time (border + text over SPI);
            // while one runs, nothing drains the host stream or the radio and
            // the RX FIFO overflows during a burst of back-to-back packets.
            // Defer the redraw while host data is pending or a radio burst is still
            // being received, so draining never falls behind either side.
            if (Serial.available() < 64 && millis() - hc11LastRxMs > 250) {
                if (screen == 0) hc11DrawStatusScreen();
                else hc11DrawByteLogScreen();
                lastDraw = now;
                redraw = false;
            }
        }

        // Fast loop: the radio FIFO must be drained often enough to keep up
        // with back-to-back bursts (e.g. the 64-packet FU1 wake-up flood).
        delay(2);
    }
}

static void hc11AboutScreen() {
    hc11DrawAboutScreen();
    unsigned long start = millis();
    while (true) {
        hc11ProcessUsb();
        if (AnyKeyPress || millis() - start > 60000) {
            check(EscPress);
            check(SelPress);
            check(NextPress);
            check(PrevPress);
            check(UpPress);
            check(DownPress);
            break;
        }
        delay(10);
    }
}

// ------------------------------------------------------------------
// App entry / teardown
// ------------------------------------------------------------------

static void hc11Teardown() {
    // Release the radio completely (ISR, IDLE, antenna path off).
    hc11RadioStop();
    // Give the CDC port back to the firmware console. The USB stack is left
    // untouched: destroying/re-initing it (massStorage-style USB.~ESPUSB() +
    // enableDFU()) drops the host connection and can wedge the OTG peripheral.
    hc11UsbActive = false;
    hc11CdcConnected = false;
    hc11ResumeConsole();
}

void hc11_modem_setup() {
    // New session: factory defaults, command mode, zero counters
    hc11Session.begin();
    hc11LineBuf = "";

    drawMainBorderWithTitle("HC-11 Modem");
    padprintln("");
    padprintln("USB CDC ready");

    // The firmware serial console reads the same CDC port - suspend it
    // while this app owns the port. The CDC itself is
    // already enumerated by the console; no USB.begin() needed.
    hc11SuspendConsole();

#if defined(ARDUINO_USB_CDC_ON_BOOT) && defined(ARDUINO_USB_MODE) && ARDUINO_USB_MODE == 1
    // HWCDC's default RX queue is only 256 bytes; a fast host stream would
    // overflow it between drain passes (and during screen redraws). Enlarge it
    // for the session -- safe to do live (the RX ISR guards a NULL queue), and
    // the console task is already suspended, so stale pending bytes are just
    // discarded. Left enlarged on exit: a bigger queue is harmless to the
    // console. TinyUSB CDC boards keep their own buffering.
    while (Serial.available() > 0) Serial.read();
    Serial.setRxBufferSize(32768);
#endif

    USB.onEvent(hc11UsbEventCallback);

    // Real RF: bring the radio up with the session's (factory) configuration.
    // The app requires the CC1101 transceiver - there is no virtual fallback:
    // without a radio, show an error and return to the menu.
    if (!hc11RadioInit(
            hc11Session.module.channel,
            hc11Session.module.address,
            hc11Session.module.txPower,
            hc11Session.module.mode,
            &hc11Session.hostToRadioBytes // TX counter: bytes actually sent
        )) {
        hc11CdcConnected = false;
        displayError("HC-11 Modem needs CC1101");
        hc11ResumeConsole();
        return;
    }
    hc11RadioSetRxCallback(hc11RadioRxDelivery, nullptr);

    hc11UsbActive = true;
    delay(300);

    // Enter the passthrough view right away: the host must get "OK" for "AT"
    // immediately after launching the app, without
    // extra on-device navigation. Esc from here opens the submenu.
    hc11PassthroughLoop();

    while (true) {
        std::vector<Option> options = {
            {"USB Passthrough", []() { hc11PassthroughLoop(); }},
            {"Configuration",   []() { hc11ConfigScreen(); }   },
            {"About",           []() { hc11AboutScreen(); }    },
            {"Back",            []() {}                        },
        };
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, "HC-11 Modem");
        if (sel < 0 || sel == 3) break; // Esc or "Back": exit app
    }

    hc11Teardown();
}

#else // !SOC_USB_OTG_SUPPORTED

#include "core/display.h"

void hc11_modem_setup() { displayError("HC-11 Modem needs native USB OTG"); }

#endif // SOC_USB_OTG_SUPPORTED
