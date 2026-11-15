#if !defined(LITE_VERSION)
#include "uart_terminal.h"

#include "core/display.h"
#include <globals.h>
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/settings.h"
#include "core/utils.h"
#include "wire_tools.h"
#include <HardwareSerial.h>

// Quick presets offered in the baudrate menu; anything else goes through "Custom"
static const uint32_t baudPresets[] = {
    1200,   2400,   4800,   9600,   14400,  19200,  28800,  38400,
    57600,  76800,  115200, 230400, 460800, 921600,
};
constexpr size_t BAUD_PRESET_COUNT = sizeof(baudPresets) / sizeof(baudPresets[0]);

// Range accepted by the ESP32 UART peripherals, same limits as the JS uart API
#define UART_BAUD_MIN 300U
#define UART_BAUD_MAX 5000000U

// Keep the terminal history bounded: drop the oldest lines beyond this count
constexpr size_t UART_TERM_MAX_LINES = 400;
// An RX run without a line terminator is flushed as a line once it gets this long
constexpr size_t UART_TERM_FLUSH_LEN = 128;
// Maximum characters typed in the input line
constexpr size_t UART_TERM_INPUT_LEN = 128;

// Kept while Bruce is running, so reopening the tool restores the last baudrate
static uint32_t uartBaud = 115200;
// Set while a serial-command UART session is open, so the UI terminal refuses to steal Serial1
static bool wireUartActive = false;

/*********************************************************************
**  Function: uart_select_baudrate
**  Preset list + custom entry (numeric keyboard), 300..5000000 bps
**********************************************************************/
static void uart_select_baudrate() {
    bool isCustom = true;
    int selected = 0;

    options.clear();
    for (size_t i = 0; i < BAUD_PRESET_COUNT; i++) {
        if (baudPresets[i] == uartBaud) {
            selected = i;
            isCustom = false;
        }
        options.push_back(
            {String(baudPresets[i]) + " bps",
             [i]() { uartBaud = baudPresets[i]; },
             baudPresets[i] == uartBaud}
        );
    }
    options.push_back(
        {"Custom",
         []() {
             String value = num_keyboard(String(uartBaud), 8, "Custom baudrate:");
             if (value == "\x1B") return;
             uint32_t baud = value.toInt();
             if (baud < UART_BAUD_MIN || baud > UART_BAUD_MAX) {
                 displayError("Baud 300 - 5000000", true);
                 return;
             }
             uartBaud = baud;
         },
         isCustom}
    );

    loopOptions(options, MENU_TYPE_SUBMENU, "Baudrate", selected);
}

/*********************************************************************
**  Function: uart_terminal_run
**  Live terminal on Serial1: shows received data, lets the user type
**  and send lines (terminated by \n). ESC leaves.
**********************************************************************/
static void uart_terminal_run() {
    if (wireUartActive) {
        displayError("UART busy (serial cmd)", true);
        return;
    }
    if (USBserial.getSerialOutput() == &Serial1) {
        displayError("(E) UART already in use", true);
        return;
    }

    BruceConfigPins::UARTPins pins = bruceConfigPins.uart_bus;
    if (pins.rx < 0 || pins.tx < 0) {
        displayError("Set RX/TX pins first", true);
        return;
    }

    Serial1.end();
    Serial1.setRxBufferSize(4096); // ride out screen redraws at high baudrates
    Serial1.begin(uartBaud, SERIAL_8N1, pins.rx, pins.tx);

    drawMainBorderWithTitle("UART Terminal");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    String info = String(uartBaud) + " 8N1  RX:" + String(pins.rx) + " TX:" + String(pins.tx);
    tft.drawString(info, BORDER_PAD_X, tft.getCursorY());

    // Last text row inside the border is reserved for the input line
    const int16_t inputY = tftHeight - BORDER_PAD_X - LH;
    const int16_t areaY = tft.getCursorY() + LH + 2;
    const int16_t areaH = inputY - 2 - areaY;
    ScrollableTextArea area(FP, BORDER_PAD_X, areaY, tftWidth - 2 * BORDER_PAD_X, areaH, false, false);
    const int16_t inputMaxChars = (tftWidth - 2 * BORDER_PAD_X) / LW - 2;

    String input = "";
    String rxRun = ""; // bytes received since the last line terminator
    bool inputDirty = true;

    auto drawInputLine = [&]() {
        tft.fillRect(BORDER_PAD_X, inputY, tftWidth - 2 * BORDER_PAD_X, LH, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        String view = input;
        if (view.length() > (size_t)inputMaxChars) view = view.substring(view.length() - inputMaxChars);
        tft.drawString(">" + view + "_", BORDER_PAD_X, inputY);
        inputDirty = false;
    };

    auto sendLine = [&]() {
        Serial1.print(input);
        Serial1.print('\n');
        area.addLine("> " + input);
        input = "";
        inputDirty = true;
    };

    while (!check(EscPress) && !returnToMenu) {
        wakeUpScreen();

        // 1) Drain everything the device sent us
        bool gotRx = false;
        bool wasAtBottom = area.lastVisibleLine >= area.getMaxLines();
        while (Serial1.available() > 0) {
            char c = Serial1.read();
            gotRx = true;
            if (c == '\r') continue;
            if (c == '\n' || rxRun.length() >= UART_TERM_FLUSH_LEN) {
                area.addLine(rxRun);
                rxRun = "";
                if (c != '\n') rxRun += c;
            } else {
                rxRun += c;
            }
        }
        if (gotRx) {
            // Bound the history, keeping the viewport in sync
            if (area.getMaxLines() > UART_TERM_MAX_LINES) {
                size_t extra = area.getMaxLines() - UART_TERM_MAX_LINES;
                area.linesBuffer.erase(area.linesBuffer.begin(), area.linesBuffer.begin() + extra);
                area.firstVisibleLine = (area.firstVisibleLine > extra) ? area.firstVisibleLine - extra : 0;
            }
            if (wasAtBottom) area.scrollToLine(area.getMaxLines());
            area.draw();
        }

        // 2) Handle the user's input
        bool sent = false;
#ifdef HAS_KEYBOARD
        keyStroke key = _getKeyPress();
        if (key.del) EscPress = false; // Backspace arrives as DEL here, it is not the exit key
        if (key.pressed) {
            for (auto c : key.word) {
                if (c == '\n' || c == '\r') continue;
                if (c == '\t' || c >= 32) {
                    if (input.length() < UART_TERM_INPUT_LEN) input += c;
                    inputDirty = true;
                }
            }
            if (key.del) {
                if (!input.isEmpty()) input.remove(input.length() - 1);
                inputDirty = true;
            }
            if (key.enter) sent = true;
        }
#else
        if (check(SelPress)) {
            String msg = keyboard(input, UART_TERM_INPUT_LEN, "Send to UART:");
            if (msg != "\x1B") {
                input = msg;
                sent = true;
            }
            area.draw(true); // modal overwrote the screen
            inputDirty = true;
        }
#endif
        if (sent) sendLine();

        // 3) Scroll the history (Up/Down, Prev/Next; auto-follows while at the bottom)
        if (check(PrevPress) || check(UpPress)) {
            area.scrollUp();
            area.draw();
        } else if (check(NextPress) || check(DownPress)) {
            area.scrollDown();
            area.draw();
        }

        if (inputDirty) drawInputLine();
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    Serial1.end();
    check(SelPress); // Enter on keyboards also raises SelPress; don't leak it into the menu
}

/*********************************************************************
**  Serial-command primitives (wire uart ...), see wire_tools.h
**********************************************************************/
bool wire_uart_begin(uint32_t baud, int8_t rx, int8_t tx) {
    if (baud < UART_BAUD_MIN || baud > UART_BAUD_MAX) return false;
    if (rx < 0) rx = (int8_t)bruceConfigPins.uart_bus.rx;
    if (tx < 0) tx = (int8_t)bruceConfigPins.uart_bus.tx;
    if (rx < 0 || tx < 0) return false;
    if (wireUartActive) wire_uart_end();

    Serial1.end();
    Serial1.setRxBufferSize(4096);
    Serial1.begin(baud, SERIAL_8N1, rx, tx);
    uartBaud = baud;
    wireUartActive = true;
    return true;
}

void wire_uart_end() {
    wireUartActive = false;
    Serial1.end();
}

int wire_uart_send(const String &text) {
    if (!wireUartActive) return -1;
    Serial1.print(text);
    return (int)text.length();
}

int wire_uart_read(String &out, uint32_t timeoutMs) {
    if (!wireUartActive) return -1;
    out = "";
    uint32_t deadline = millis() + timeoutMs;
    while ((int32_t)(millis() - deadline) < 0) {
        while (Serial1.available() > 0) out += (char)Serial1.read();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return (int)out.length();
}

String wire_uart_status() {
    BruceConfigPins::UARTPins pins = bruceConfigPins.uart_bus;
    return String("baud=") + uartBaud + " rx=" + int(pins.rx) + " tx=" + int(pins.tx) + " active=" +
           (wireUartActive ? "1" : "0");
}

/*********************************************************************
**  Function: uart_terminal_setup
**  Configuration menu: baudrate (presets/custom), pins and start
**********************************************************************/
void uart_terminal_setup() {
    returnToMenu = false;
    while (!returnToMenu) {
        BruceConfigPins::UARTPins &pins = bruceConfigPins.uart_bus;
        options = {
            {String("Baudrate: ") + uartBaud, uart_select_baudrate},
            {String("Pins: RX ") + int(pins.rx) + " TX " + int(pins.tx),
             []() { setUARTPinsMenu(bruceConfigPins.uart_bus); }            },
            {"Start Terminal", uart_terminal_run                          },
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "UART Terminal");
    }
}
#endif
