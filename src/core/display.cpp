#include "display.h"
#include "core/wifi/webInterface.h" // for server
#include "core/wifi/wg.h"           //for isConnectedWireguard to print wireguard lock
#include "mykeyboard.h"
#include "settings.h" //for timeStr
#include "utils.h"
#include <JPEGDecoder.h>
#include <interface.h> //for charging ischarging to print charging indicator
#include <memory>

#define MAX_MENU_SIZE (int)(tftHeight / 25)

// Send the ST7789 into or out of sleep mode
void panelSleep(bool on) {
#if defined(ST7789_2_DRIVER) || defined(ST7789_DRIVER)
    if (on) {
        tft.writecommand(0x10); // SLPIN: panel off
        delay(5);
    } else {
        tft.writecommand(0x11); // SLPOUT: panel on
        delay(120);
    }
#endif
    // Disables tft writings on the display
    tft.setSleepMode(on);
}

bool __attribute__((weak)) isCharging() { return false; }
/***************************************************************************************
** Function name: displayScrollingText
** Description:   Scroll large texts into screen
***************************************************************************************/
void displayScrollingText(const String &text, Opt_Coord &coord) {
    int len = text.length();
    String displayText = text + "        "; // Add spaces for smooth looping
    int scrollLen = len + 8;                // Full text plus space buffer
    static int i = 0;
    static long _lastmillis = 0;
    tft.setTextColor(coord.fgcolor, coord.bgcolor);
    if (len < coord.size) {
        // Text fits within limit, no scrolling needed
        return;
    } else if (millis() > _lastmillis + 200) {
        String scrollingPart =
            displayText.substring(i, i + (coord.size - 1)); // Display charLimit characters at a time
        tft.fillRect(
            coord.x, coord.y, (coord.size - 1) * LW * tft.textsize, LH * tft.textsize, bruceConfig.bgColor
        ); // Clear display area
        tft.setCursor(coord.x, coord.y);
        tft.setCursor(coord.x, coord.y);
        tft.print(scrollingPart);
        if (i >= scrollLen - coord.size) i = -1; // Loop back
        _lastmillis = millis();
        i++;
        if (i == 1) _lastmillis = millis() + 1000;
    }
}

/***************************************************************************************
** Function name: TouchFooter
** Description:   Draw touch screen footer
***************************************************************************************/
void TouchFooter(uint16_t color) {
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);
    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.drawCentreString("PREV", tftWidth / 6, tftHeight + 4, 1);
    tft.drawCentreString("SEL", tftWidth / 2, tftHeight + 4, 1);
    tft.drawCentreString("NEXT", 5 * tftWidth / 6, tftHeight + 4, 1);
}
/***************************************************************************************
** Function name: TouchFooter
** Description:   Draw touch screen footer
***************************************************************************************/
void MegaFooter(uint16_t color) {
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);
    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.drawCentreString("Exit", tftWidth / 6, tftHeight + 4, 1);
    tft.drawCentreString("UP", tftWidth / 2, tftHeight + 4, 1);
    tft.drawCentreString("DOWN", 5 * tftWidth / 6, tftHeight + 4, 1);
}

/***************************************************************************************
** Function name: resetTftDisplay
** Description:   set cursor to 0,0, screen and text to default color
***************************************************************************************/
void resetTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg, uint16_t screen) {
    tft.setCursor(x, y);
    tft.fillScreen(screen);
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

/***************************************************************************************
** Function name: setTftDisplay
** Description:   set cursor, font color, size and bg font color
***************************************************************************************/
void setTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg) {
    if (x >= 0 && y < 0) tft.setCursor(x, tft.getCursorY());      // if -1 on x, sets only y
    else if (x < 0 && y >= 0) tft.setCursor(tft.getCursorX(), y); // if -1 on y, sets only x
    else if (x >= 0 && y >= 0) tft.setCursor(x, y);               // if x and y > 0, sets both
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

void turnOffDisplay() { setBrightness(0, false); }

bool wakeUpScreen() {
    previousMillis = millis();
    if (isScreenOff) {
        isScreenOff = false;
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    } else if (dimmer) {
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    }
    return false;
}

/***************************************************************************************
** Function name: displayRedStripe
** Description:   Display Red Stripe with information
***************************************************************************************/
void displayRedStripe(String text, uint16_t fgcolor, uint16_t bgcolor) {
    // detect if not running in interactive mode -> show nothing onscreen and return immediately
    // if (server || isSleeping || isScreenOff) return; // webui is running

    int size;
    if (fgcolor == bgcolor && fgcolor == TFT_WHITE) fgcolor = TFT_BLACK;
    if (text.length() * LW * FM < (tftWidth - 2 * FM * LW)) size = FM;
    else size = FP;
    tft.drawPixel(0, 0, 0);
    tft.fillRoundRect(10, tftHeight / 2 - 13, tftWidth - 20, 26, 7, bgcolor);
    tft.setTextColor(fgcolor, bgcolor);
    if (size == FM) {
        tft.setTextSize(FM);
        tft.drawCentreString(text, tftWidth / 2, tftHeight / 2 - 8);
    } else {
        tft.setTextSize(FP);
        int text_size = text.length();
        if (text_size < (tftWidth - 20) / (LW * FP))
            tft.drawCentreString(text, tftWidth / 2, tftHeight / 2 - 8);
        else {
            tft.drawCentreString(text.substring(0, text_size / 2), tftWidth / 2, tftHeight / 2 - 9);
            tft.drawCentreString(text.substring(text_size / 2), tftWidth / 2, tftHeight / 2 + 1);
        }
    }
}

void drawButton(
    int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, const char *text, bool inverted = false
) {
    if (inverted) {
        tft.fillRoundRect(x, y, w, h, 5, color);
    } else {
        tft.fillRoundRect(x, y, w, h, 5, TFT_BLACK);
        tft.drawRoundRect(x, y, w, h, 5, color);
    }
    tft.setTextColor(inverted ? TFT_BLACK : color);
    tft.drawString(text, x + w / 2, y + h);
}

int8_t displayMessage(
    const char *message, const char *leftButton, const char *centerButton, const char *rightButton,
    uint16_t color
) {
#ifdef HAS_SCREEN
    uint8_t oldTextDatum = tft.getTextDatum();
#endif

    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(message, tftWidth / 2, tftHeight / 2 - 20);

    tft.setTextDatum(BC_DATUM);
    int16_t buttonHeight = 20;
    int16_t buttonY = tftHeight - buttonHeight - 5;
    int16_t buttonWidth = tftWidth / 3 - 10;

    int8_t totalButtons = (leftButton ? 1 : 0) + (centerButton ? 1 : 0) + (rightButton ? 1 : 0);
    int8_t selected = 0; // Start at first available button
    bool redraw = true;

    while (true) {
        if (check(PrevPress) || check(EscPress)) {
            selected = (selected - 1 + totalButtons) % totalButtons; // Cycle backward
            redraw = true;
        }
        if (check(NextPress)) {
            selected = (selected + 1) % totalButtons; // Cycle forward
            redraw = true;
        }
        if (check(SelPress)) { break; }

        // Draw buttons with selection highlighting
        int8_t index = 0;

        if (redraw) {
            if (leftButton) {
                drawButton(5, buttonY, buttonWidth, buttonHeight, color, leftButton, selected == index);
                index++;
            }

            if (centerButton) {
                drawButton(
                    tftWidth / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    centerButton,
                    selected == index
                );
                index++;
            }

            if (rightButton) {
                drawButton(
                    tftWidth * 2 / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    rightButton,
                    selected == index
                );
            }
            redraw = false;
        }

        delay(10);
    }

#ifdef HAS_SCREEN
    tft.setTextDatum(oldTextDatum);
#endif

    return selected;
}

void displayError(String txt, bool waitKeyPress) {
    displayRedStripe(txt);
#ifndef HAS_SCREEN
    Serial.println("ERR: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayWarning(String txt, bool waitKeyPress) {
    displayRedStripe(txt, TFT_BLACK, TFT_YELLOW);
#ifndef HAS_SCREEN
    Serial.println("WARN: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayInfo(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, TFT_WHITE, TFT_BLUE);
#ifndef HAS_SCREEN
    Serial.println("INFO: " + txt);
    return;
#endif

    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displaySuccess(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, TFT_WHITE, TFT_DARKGREEN);
#ifndef HAS_SCREEN
    Serial.println("SUCCESS: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayTextLine(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
#ifndef HAS_SCREEN
    Serial.println("MESSAGE: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void setPadCursor(int16_t padx, int16_t pady) {
    for (int y = 0; y < pady; y++) tft.println();
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
}

void padprintf(int16_t padx, const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}
void padprintf(const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    tft.setCursor(BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}

void padprint(const String &s, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(s);
}
void padprint(const char str[], int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(str);
}
void padprint(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(c);
}
void padprint(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(b, base);
}
void padprint(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, digits);
}

void padprintln(const String &s, int16_t padx) {
    if (s.isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(s);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // automatically split into multiple lines
    while (!(buff = s.substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(const char str[], int16_t padx) {
    if (strcmp(str, "") == 0) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(str);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // automatically split into multiple lines
    while (!(buff = String(str).substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(c);
}
void padprintln(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(b, base);
}
void padprintln(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, digits);
}

/*********************************************************************
**  Function: loopOptions
**  Where you choose among the options in menu
**********************************************************************/
int loopOptions(
    std::vector<Option> &options, uint8_t menuType, const char *subText, int index, bool interpreter
) {
    Opt_Coord coord;
    bool redraw = true;
    bool exit = false;
    int menuSize = options.size();
    int devModeCounter = 0;
    static unsigned long _clock_bat_timer = millis();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }
    if (index > 0)
        tft.fillRoundRect(
            tftWidth * 0.10,
            tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5,
            tftWidth * 0.8,
            (FM * 8 + 4) * menuSize + 10,
            5,
            bruceConfig.bgColor
        );
    if (index >= options.size()) index = 0;
    bool firstRender = true;
    drawMainBorder();
    while (1) {
        // Check for shutdown before drawing menu to avoid drawing a black bar on the screen
        if (exit) break;
        if (menuType == MENU_TYPE_MAIN) {
            checkReboot();
            if (devModeCounter >= 5 && !bruceConfig.devMode) {
                bruceConfig.setDevMode(true);
                displayInfo("Dev Mode Enabled", true);
            }
            if (millis() - _clock_bat_timer > 30000) {
                _clock_bat_timer = millis();
                drawStatusBar(); // update clock and battery status each 30s
            }
        }

        if (redraw) {
            menuOptionType = menuType; // updates menutype to the remote controller
            menuOptionLabel = subText;
            // update the hovered
            for (auto &opt : options) opt.hovered = false;
            options[index].hovered = true;

            bool renderedByLambda = false;
            if (options[index].hover)
                renderedByLambda = options[index].hover(options[index].hoverPointer, true);

            if (!renderedByLambda) {
                if (menuType == MENU_TYPE_SUBMENU) drawSubmenu(index, options, subText);
                else
                    coord = drawOptions(
                        index,
                        options,
                        bruceConfig.priColor,
                        bruceConfig.secColor,
                        bruceConfig.bgColor,
                        firstRender
                    );
            }
            firstRender = false;
            redraw = false;
        }

        // handleSerialCommands(); // always use serial task for it
#ifdef HAS_KEYBOARD
        checkShortcutPress(); // shortctus to quickly start apps without navigating the menus
#endif

        if (menuType == MENU_TYPE_REGULAR) {
            String txt = options[index].label;
            displayScrollingText(txt, coord);
        }

        // Checks ESC Press first, to not exit after PrevPress is processed
        // PrevPress condition is a StickCPlus workaround, as it uses the same button for Prev and Esc
        // Same happens to Core and some other boards
        if (EscPress && PrevPress) EscPress = false;
        if (menuType != MENU_TYPE_MAIN && check(EscPress)) {
            index = -1;
            break;
        }

        if (PrevPress || check(UpPress)) {
            devModeCounter = 0;
#ifdef HAS_KEYBOARD
            check(PrevPress);
            if (index == 0) index = options.size() - 1;
            else if (index > 0) index--;
            redraw = true;
#else
            long _tmp = millis();
#ifndef HAS_ENCODER // T-Embed doesn't need it
            LongPress = true;
            while (PrevPress && menuType != MENU_TYPE_MAIN) {
                if (millis() - _tmp > 200)
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        360 * (millis() - (_tmp + 200)) / 500,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                vTaskDelay(10 / portTICK_RATE_MS);
            }
            tft.drawArc(
                tftWidth / 2, tftHeight / 2, 25, 15, 0, 360, bruceConfig.bgColor, bruceConfig.bgColor
            );
            LongPress = false;
#endif
            if (millis() - _tmp > 700) { // longpress detected to exit
                index = -1;
                break;
            } else {
                check(PrevPress);
                if (index == 0) index = options.size() - 1;
                else if (index > 0) index--;
                redraw = true;
            }
#endif
        }
        /* DW Btn to next item */
        if (check(NextPress) || check(DownPress)) {
            index++;
            if ((index + 1) > options.size()) {
                if (!bruceConfig.devMode) devModeCounter++;
                index = 0;
            }
            redraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);

        /* Select and run function
        forceMenuOption is set by a SerialCommand to force a selection within the menu
        */
        if (check(SelPress) || forceMenuOption >= 0) {
            uint16_t chosen = index;
            if (forceMenuOption >= 0) {
                chosen = forceMenuOption;
                forceMenuOption = -1; // reset SerialCommand navigation option
                Serial.print("Forcely ");
            }
            Serial.println("Selected: " + String(options[chosen].label));
            options[chosen].operation();
            break;
        }
        // interpreter_start -> running the interpreter
        // interpreter -> loopOptions helper inside the Javascript
        if (interpreter_start && !interpreter) { break; }
    }
    return index;
}

/***************************************************************************************
** Function name: progressHandler
** Description:   Função para manipular o progresso da atualização
** Dependencia: prog_handler =>>    0 - Flash, 1 - LittleFS
***************************************************************************************/
void progressHandler(int progress, size_t total, String message) {
    int barWidth = map(progress, 0, total, 0, tftWidth - 40);
    if (barWidth < 3) {
        tft.fillRect(6, 27, tftWidth - 12, tftHeight - 33, bruceConfig.bgColor);
        tft.drawRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.priColor);
        displayRedStripe(message, TFT_WHITE, bruceConfig.priColor);
    }
    tft.fillRect(20, tftHeight - 45, barWidth, 13, bruceConfig.priColor);
}

/***************************************************************************************
** Function name: drawOptions
** Description:   Função para desenhar e mostrar as opçoes de contexto
***************************************************************************************/
Opt_Coord drawOptions(
    int index, std::vector<Option> &options, uint16_t fgcolor, uint16_t selcolor, uint16_t bgcolor,
    bool firstRender
) {
    Opt_Coord coord;
    int menuSize = options.size();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }

    // Uncomment to update the statusBar (causes flickering)
    // drawStatusBar();

    int32_t optionsTopY = tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5;
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    if (firstRender) {
        tft.fillRoundRect(
            tftWidth * 0.10, optionsTopY, tftWidth * 0.8, (FM * 8 + 4) * menuSize + 10, 5, bgcolor
        );
        tft.drawRoundRect(
            tftWidth * 0.10,
            tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5,
            tftWidth * 0.8,
            (FM * 8 + 4) * menuSize + 10,
            5,
            fgcolor
        );
    }

    tft.setTextColor(fgcolor, bgcolor);
    tft.setTextSize(FM);
    tft.setCursor(tftWidth * 0.10 + 5, tftHeight / 2 - menuSize * (FM * 8 + 4) / 2);

    int i = 0;
    int init = 0;
    int cont = 1;
    menuSize = options.size();
    if (index >= MAX_MENU_SIZE) init = index - MAX_MENU_SIZE + 1;
    for (i = 0; i < menuSize; i++) {
        if (i >= init) {
            if (options[i].selected) tft.setTextColor(selcolor, bgcolor); // if selected, change Text color
            else tft.setTextColor(fgcolor, bgcolor);

            String text = "";
            if (i == index) {
                text += ">";
                coord.x = tftWidth * 0.10 + 5 + FM * LW;
                coord.y = tft.getCursorY() + 4;
                coord.size = (tftWidth * 0.8 - 10) / (LW * FM) - 1;
                coord.fgcolor = fgcolor;
                coord.bgcolor = bgcolor;
            } else text += " ";
            text += String(options[i].label) + "              ";
            tft.setCursor(tftWidth * 0.10 + 5, tft.getCursorY() + 4);
            tft.println(text.substring(0, (tftWidth * 0.8 - 10) / (LW * FM) - 1));
            cont++;
        }
        if (cont > MAX_MENU_SIZE) goto Exit;
    }
Exit:
    if (options.size() > MAX_MENU_SIZE) menuSize = MAX_MENU_SIZE;
#if defined(HAS_TOUCH)
    TouchFooter();
#endif
    return coord;
}

/***************************************************************************************
** Function name: drawSubmenu
** Description:   Função para desenhar e mostrar as opçoes de contexto
***************************************************************************************/
void drawSubmenu(int index, std::vector<Option> &options, const char *title) {
    drawStatusBar();
    int menuSize = options.size();
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawPixel(0, 0, 0);
    tft.fillRect(6, 30, tftWidth - 12, 8 * FP, bruceConfig.bgColor);
    tft.drawString(title, 12, 30);

    // middle of the drawing area
    int middle = 25 /*status*/ + (tftHeight - 30 /*status + bottom margin*/) / 2;
    // drawCentreString uses TC_DATUM, so we need to adjust the Y position
    // 42 ensures that title isnt touched( 30 + 8 (LH) + 4(Margin))
    int middle_up = middle - (tftHeight - 42) / 3 - FM * LH / 2 + 4;
    int middle_down = middle + (tftHeight - 42) / 3 - FM * LH / 2;

    tft.setTextSize(FM);
#if defined(HAS_TOUCH)
    tft.drawCentreString("/\\", tftWidth / 2, middle_up - (FM * LH + 6), 1);
#endif
    // Previous item
    const char *firstOption =
        index - 1 >= 0 ? options[index - 1].label.c_str() : options[menuSize - 1].label.c_str();
    tft.setTextColor(bruceConfig.secColor);
    tft.fillRect(6, middle_up, tftWidth - 12, 8 * FM, bruceConfig.bgColor);
    tft.drawCentreString(firstOption, tftWidth / 2, middle_up, SMOOTH_FONT);

    // Selected item
    int selectedTextSize = options[index].label.length() <= tftWidth / (LW * FG) - 1 ? FG : FM;
    tft.setTextSize(selectedTextSize);
    tft.setTextColor(bruceConfig.priColor);
    tft.fillRect(6, middle - FG * LH / 2 - 1, tftWidth - 12, FG * LH + 5, bruceConfig.bgColor);
    tft.drawCentreString(options[index].label, tftWidth / 2, middle - selectedTextSize * LH / 2, SMOOTH_FONT);
    tft.drawFastHLine(
        tftWidth / 2 - strlen(options[index].label.c_str()) * selectedTextSize * LW / 2,
        middle + selectedTextSize * LH / 2 + 1,
        strlen(options[index].label.c_str()) * selectedTextSize * LW,
        bruceConfig.priColor
    );
    // Next Item
    const char *thirdOption =
        index + 1 < menuSize ? options[index + 1].label.c_str() : options[0].label.c_str();
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.secColor);
    tft.fillRect(6, middle_down, tftWidth - 12, 8 * FM, bruceConfig.bgColor);
    tft.drawCentreString(thirdOption, tftWidth / 2, middle_down, SMOOTH_FONT);

    tft.fillRect(tftWidth - 5, 0, 5, tftHeight, bruceConfig.bgColor);
    tft.fillRect(tftWidth - 5, index * tftHeight / menuSize, 5, tftHeight / menuSize, bruceConfig.priColor);

#if defined(HAS_TOUCH)
    tft.drawCentreString("\\/", tftWidth / 2, middle_down + (FM * LH + 6), 1);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.drawString("[ x ]", 7, 7, 1);
    TouchFooter();
#endif
}

void drawStatusBar() {
    int i = 0;
    uint8_t bat = getBattery();
    uint8_t bat_margin = 85;
    if (bat > 0) {
        drawBatteryStatus(bat);
    } else bat_margin = 26;
    if (sdcardMounted) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.drawString("SD", tftWidth - (bat_margin), 12);
        i++;
    } // Indication for SD card on screen
    if (gpsConnected) {
        drawGpsSmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    }
    if (WiFi.getMode()) {
        drawWifiSmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    } // Draw Wifi Symbol beside battery
    if (isWebUIActive) {
        drawWebUISmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    } // Draw Wifi Symbol beside battery
    if (BLEConnected) {
        drawBLESmall(tftWidth - (bat_margin + 23 * i), 7);
        i++;
    } // Draw BLE beside Wifi
    if (isConnectedWireguard) {
        drawWireguardStatus(tftWidth - (bat_margin + 24 * i), 7);
        i++;
    } // Draw Wg bedide BLE, if the others exist, if not, beside battery

    if (bruceConfig.theme.border) {
        tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
        tft.drawLine(5, 25, tftWidth - 6, 25, bruceConfig.priColor);
    }

    if (clock_set) {
        int clock_fontsize = 1; // Font size of the clock / BRUCE + BRUCE_VERSION
        setTftDisplay(12, 12, bruceConfig.priColor, clock_fontsize, bruceConfig.bgColor);
#if defined(HAS_RTC)
        _rtc.GetTime(&_time);
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", _time.Hours, _time.Minutes);
        tft.print(timeStr);
#else
        updateTimeStr(rtc.getTimeStruct());
        tft.fillRect(12, 12, 100, clock_fontsize * LH, bruceConfig.bgColor);
        tft.print(timeStr);
#endif
    } else {
        setTftDisplay(12, 12, bruceConfig.priColor, 1, bruceConfig.bgColor);
        tft.print("BRUCE " + String(BRUCE_VERSION));
    }
}

void drawMainBorder(bool clear) {
    if (clear) {
        tft.drawPixel(0, 0, 0);
        tft.fillScreen(bruceConfig.bgColor);
    }
    setTftDisplay(12, 12, bruceConfig.priColor, 1, bruceConfig.bgColor);
    tft.setTextDatum(0);

    // if(wifiConnected) {tft.print(timeStr);} else {tft.print("BRUCE 1.0b");}

    drawStatusBar();

#if defined(HAS_TOUCH)
    TouchFooter();
#endif
}

void drawMainBorderWithTitle(String title, bool clear) {
    drawMainBorder(clear);
    printTitle(title);
}

void printTitle(String title) {
    tft.setCursor((tftWidth - (title.length() * FM * LW)) / 2, BORDER_PAD_Y);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);

    title.toUpperCase();
    tft.println(title);

    tft.setTextSize(FP);
}

void printSubtitle(String subtitle, bool withLine) {
    int16_t cursorX = (tftWidth - (subtitle.length() * FP * LW)) / 2;
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    tft.setCursor(cursorX, BORDER_PAD_Y + FM * LH);
    tft.println(subtitle);

    if (withLine) {
        String line = "";
        for (byte i = 0; i < subtitle.length(); i++) line += "-";

        tft.setCursor(cursorX, tft.getCursorY());
        tft.println(line);
    }
}

void printFootnote(String text) {
    tft.setTextSize(FP);
    tft.drawRightString(text, tftWidth - BORDER_PAD_X, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

void printCenterFootnote(String text) {
    tft.fillRect(10, tftHeight - BORDER_PAD_X - FP * LH, tftWidth - 20, FP * LH, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(text, tftWidth / 2, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

/***************************************************************************************
** Function name: drawBatteryStatus()
** Description:   Draws battery info into the Status bar
***************************************************************************************/
void drawBatteryStatus(uint8_t bat) {
    if (bat == 0) return;

    bool charging = isCharging();

    uint16_t color = bruceConfig.priColor;
    uint16_t barcolor = bruceConfig.priColor;
    if (bat < 16) barcolor = color = TFT_RED;
    else if (bat < 34) barcolor = color = TFT_YELLOW;
    if (charging) color = TFT_GREEN;

    tft.drawRoundRect(tftWidth - 43, 6, 36, 19, 2, charging ? color : bruceConfig.bgColor); // (bolder border)
    tft.drawRoundRect(tftWidth - 42, 7, 34, 17, 2, color);
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawRightString((bat == 100 ? "" : " ") + String(bat) + "%", tftWidth - 45, 12, 1);
    tft.fillRoundRect(tftWidth - 40, 9, 30 * bat / 100, 13, 2, barcolor);
    tft.drawLine(tftWidth - 30, 9, tftWidth - 30, 9 + 13, bruceConfig.bgColor);
    tft.drawLine(tftWidth - 20, 9, tftWidth - 20, 9 + 13, bruceConfig.bgColor);
}
/***************************************************************************************
** Function name: drawWireguardStatus()
** Description:   Draws a padlock when connected
***************************************************************************************/
void drawWireguardStatus(int x, int y) {
    tft.fillRect(x, y, 20, 17, bruceConfig.bgColor);
    if (isConnectedWireguard) {
        tft.drawRoundRect(11 + x, 0 + y, 8, 12, 5, TFT_GREEN);
        tft.fillRoundRect(10 + x, 8 + y, 10, 8, 0, TFT_GREEN);
    } else {
        tft.drawRoundRect(1 + x, 0 + y, 8, 12, 5, bruceConfig.priColor);
        tft.fillRoundRect(0 + x, 8 + y, 10, 8, 0, bruceConfig.bgColor);
        tft.fillRoundRect(6 + x, 8 + y, 10, 10, 0, bruceConfig.priColor);
    }
}

/***************************************************************************************
** Function name: listFiles
** Description:   Função para desenhar e mostrar o menu principal
***************************************************************************************/
#define MAX_ITEMS (int)(tftHeight - 20) / (LH * FM)
Opt_Coord listFiles(int index, std::vector<FileList> fileList) {
    Opt_Coord coord;
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    if (index == 0) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
    }
    tft.setCursor(10, 10);
    tft.setTextSize(FM);
    int i = 0;
    int arraySize = fileList.size();
    int start = 0;
    if (index >= MAX_ITEMS) {
        start = index - MAX_ITEMS + 1;
        if (start < 0) start = 0;
    }
    int nchars = (tftWidth - 20) / (6 * tft.textsize);
    String txt = ">";
    while (i < arraySize) {
        if (i >= start) {
            tft.setCursor(10, tft.getCursorY());
            if (fileList[i].folder == true)
                tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
            else if (fileList[i].operation == true) tft.setTextColor(ALCOLOR, bruceConfig.bgColor);
            else { tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor); }

            if (index == i) {
                txt = ">";
                coord.x = 10 + FM * LW;
                coord.y = tft.getCursorY();
                coord.size = nchars;
                coord.fgcolor =
                    fileList[i].folder ? getColorVariation(bruceConfig.priColor) : bruceConfig.priColor;
                coord.bgcolor = bruceConfig.bgColor;
            } else txt = " ";
            txt += fileList[i].filename + "                 ";
            tft.println(txt.substring(0, nchars));
        }
        i++;
        if (i == (start + MAX_ITEMS) || i == arraySize) break;
    }
    return coord;
}

// desenhos do menu principal, sprite "draw" com 80x80 pixels

void drawWifiSmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);
    tft.fillCircle(9 + x, 14 + y, 1, bruceConfig.priColor);
    tft.drawArc(9 + x, 14 + y, 4, 6, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(9 + x, 14 + y, 10, 12, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
}

void drawWebUISmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);

    tft.drawCircle(8 + x, 8 + y, 7, bruceConfig.priColor);

    tft.drawLine(3 + x, 4 + y, 14 + x, 4 + y, bruceConfig.priColor);
    tft.drawLine(2 + x, 8 + y, 15 + x, 8 + y, bruceConfig.priColor);
    tft.drawLine(3 + x, 12 + y, 14 + x, 12 + y, bruceConfig.priColor);
}

void drawBLESmall(int x, int y) {
    tft.fillRect(x, 2 + y, 17, 13, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 5 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 13 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 2 + y, 13 + x, 5 + y, bruceConfig.priColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 14 + y, 13 + x, 11 + y, bruceConfig.priColor);
}

void drawBLE_beacon(int x, int y, uint16_t color) {
    tft.fillRect(x, y, 40, 80, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 2 + x, 26 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 2 + x, 53 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 20 + x, 12 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(20 + x, 12 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 26 + y, 20 + x, 40 + y, 20 + x, 12 + y, color);
    tft.fillTriangle(40 + x, 53 + y, 20 + x, 40 + y, 20 + x, 68 + y, color);
}

void drawGPS(int x, int y) {
    tft.fillRect(x, y, 80, 80, bruceConfig.bgColor);
    tft.drawEllipse(40 + x, 70 + y, 15, 8, bruceConfig.priColor);
    tft.drawArc(40 + x, 25 + y, 23, 7, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 70 + y, 20 + x, 64 + y, 60 + x, 64 + y, bruceConfig.priColor);
}

void drawGpsSmall(int x, int y) {
    tft.fillRect(x, y, 17, 17, bruceConfig.bgColor);
    tft.drawEllipse(9 + x, 14 + y, 4, 3, bruceConfig.priColor);
    tft.drawArc(9 + x, 6 + y, 5, 2, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(9 + x, 15 + y, 5 + x, 9 + y, 13 + x, 9 + y, bruceConfig.priColor);
}

void drawCreditCard(int x, int y) {
    tft.fillRect(x, y, 70, 50, bruceConfig.bgColor);
    tft.fillRoundRect(x + 5, y + 5, 60, 40, 5, bruceConfig.priColor);
    tft.fillRect(x + 5, y + 15, 60, 10, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.fillRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 3, 1));
    tft.drawRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10 + 4, y + 30, 4, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 17, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.fillRect(x + 30, y + 35, 30, 5, getColorVariation(bruceConfig.priColor, 5, 1));
}

void drawMfkey32Icon(int x, int y) {
    tft.drawRect(x + 2, y + 15, 24, 40, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 18, 18, 12, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 34, 18, 18, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 40, x + 22, y + 40, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 46, x + 22, y + 46, bruceConfig.priColor);
    tft.drawLine(x + 11, y + 34, x + 11, y + 51, bruceConfig.priColor);
    tft.drawLine(x + 17, y + 34, x + 17, y + 51, bruceConfig.priColor);
    tft.drawRect(x + 30, y + 10, 25, 35, bruceConfig.priColor);
    int startX = x + 32;
    int startY = y + 12;
    int endX = x + 52;
    int endY = y + 32;
    int step = 2;
    int turns = 0;

    while (startX <= endX && startY <= endY && turns < 3) {
        for (int i = startX; i <= endX; i++) { tft.drawPixel(i, startY, bruceConfig.priColor); }
        startY += step;
        for (int i = startY; i <= endY; i++) { tft.drawPixel(endX, i, bruceConfig.priColor); }
        endX -= step;
        for (int i = endX; i >= startX; i--) { tft.drawPixel(i, endY, bruceConfig.priColor); }
        endY -= step;
        for (int i = endY; i >= startY; i--) { tft.drawPixel(startX, i, bruceConfig.priColor); }
        startX += step;
        turns++;
    }
    tft.fillRect(x + 40, y + 36, 6, 6, getColorVariation(bruceConfig.priColor, 3, 1));
}

void drawMfkey64Icon(int x, int y) {
    drawMfkey32Icon(x, y);
    tft.fillRoundRect(x + 40, y + 6, 24, 14, 4, bruceConfig.bgColor);
    tft.drawRoundRect(x + 40, y + 6, 24, 14, 4, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.drawCircle(x + 48, y + 12, 4, getColorVariation(bruceConfig.priColor, 3, -1));
}

// ####################################################################################################
//  Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// ####################################################################################################
//  from:
//  https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/ESP32_SDcard_jpeg/ESP32_SDcard_jpeg.ino
//  This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
//  fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

    // jpegInfo(); // Print information from the JPEG file (could comment this line out)

    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
    uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // Fetch data from the file, decode and display
    tft.fillRect(xpos, ypos, JpegDec.width, JpegDec.height, TFT_BLACK);
    while (JpegDec.read()) {   // While there is more data in the file
        pImg = JpegDec.pImage; // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

        // Calculate coordinates of top left corner of current MCU
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w) {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++) {
                p += mcu_w;
                for (int w = 0; w < win_w; w++) {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
            tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        else if ((mcu_y + win_h) > tft.height())
            JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }

    tft.setSwapBytes(swapBytes);
}

bool showJpeg(FS &fs, String filename, int x, int y, bool center) {
    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();
    File picture;
    if (fs.exists(filename)) picture = fs.open(filename, FILE_READ);
    else return false;

    const size_t data_size = picture.size();

    // Alloc memory into heap
    uint8_t *data_array = new uint8_t[data_size];
    if (data_array == nullptr) {
        // Fail allocating memory
        picture.close();
        delete[] data_array;
        return false;
    }

    uint8_t data;
    int i = 0;
    byte line_len = 0;

    while (picture.available()) {
        data = picture.read();
        data_array[i] = data;
        i++;

        // print array on Serial
        /*
        Serial.print("0x");
        if (abs(data) < 16) {
          Serial.print("0");
        }

        Serial.print(data, HEX);
        Serial.print(","); // Add value and comma
        line_len++;
        if (line_len >= 32) {
          line_len = 0;
          Serial.println();
        }
        */
    }

    picture.close();

    bool decoded = false;
    if (data_array) {
        decoded = JpegDec.decodeArray(data_array, data_size);
    } else {
        displayError(filename + " Fail");
        delay(2500);
        delete[] data_array; // free heap before leaving
        return false;
    }

    if (decoded) {
        if (center) {
            x = x + (tftWidth - JpegDec.width) / 2;
            y = y + (tftHeight - JpegDec.height) / 2;
        }
        jpegRender(x, y);
    }
    // calculate how long it took to draw the image
    drawTime = millis() - drawTime; // Calculate the time it took

    // print the results to the serial port
    Serial.print("Total render time was    : ");
    Serial.print(drawTime);
    Serial.println(" ms");
    Serial.println("=====================================");

    delete[] data_array; // free heap before leaving
    return true;
}

#if !defined(LITE_VERSION)
// ####################################################################################################
//  Draw a GIF on the TFT
//  derived from
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/TFT_eSPI_memory/TFT_eSPI_memory.ino and
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/best_practices_example/best_practices_example.ino
// ####################################################################################################

Gif::Gif() : gifPosition(0, 0) {}

Gif::~Gif() {
    gif->close();
    delete gif;
}

FS *Gif::GifFs = NULL;

void *Gif::openFile(const char *fname, int32_t *pSize) {
    File GifFile;

    if (GifFs != NULL) {
        GifFile = GifFs->open(fname);
    } else {
        if (SD.exists(fname)) GifFile = SD.open(fname);
        else if (LittleFS.exists(fname)) GifFile = LittleFS.open(fname);
    }

    File *FSGifFile = new File(GifFile);

    if (FSGifFile) {
        *pSize = FSGifFile->size();
        return (void *)FSGifFile;
    }
    return NULL;
}

void Gif::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) { f->close(); }
    delete f;
}

int32_t Gif::readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0) return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
}

int32_t Gif::seekFile(GIFFILE *pFile, int32_t iPosition) {
    int i = micros();
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    i = micros() - i;
    return pFile->iPos;
}

void Gif::GIFDraw(GIFDRAW *pDraw) {
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[tftWidth];
    int x, y, iWidth;

    GifPosition *position = (GifPosition *)(pDraw->pUser);

    iWidth = pDraw->iWidth;
    if (iWidth > tftWidth) iWidth = tftWidth;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) { // restore to background color
        for (x = 0; x < iWidth; x++) {
            if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) { // if transparency used
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while (x < iWidth) {
            c = ucTransparent - 1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) { // done, stop
                    s--;                  // back up to treat it like transparent
                } else {                  // opaque
                    *d++ = usPalette[c];
                    iCount++;
                }
            } // while looking for opaque pixels
            if (iCount) { // any opaque pixels?
                tft.drawPixel(0, 0, 0);
                tft.pushImage(pDraw->iX + x + position->x, y + position->y, iCount, 1, (uint16_t *)usTemp);
                x += iCount;
                iCount = 0;
            }
            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) iCount++;
                else s--;
            }
            if (iCount) {
                x += iCount; // skip these
                iCount = 0;
            }
        }
    } else {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x = 0; x < iWidth; x++) usTemp[x] = usPalette[*s++];
        tft.drawPixel(0, 0, 0);
        tft.pushImage(pDraw->iX + position->x, y + position->y, iWidth, 1, (uint16_t *)usTemp);
    }
} /* GIFDraw() */

bool Gif::openGIF(FS *fs, const char *filename) {
    if (fs != NULL) {
        GifFs = fs;
        if (!fs->exists(filename)) return false;
    } else {
        GifFs = NULL;
    }

    gif = new AnimatedGIF();
    gif->begin(BIG_ENDIAN_PIXELS);
    if (gif->open(filename, openFile, closeFile, readFile, seekFile, GIFDraw)) { return true; }

    log_e("GIF opening error: %d\n", gif->getLastError());
    return false;
}

// Play a single frame
// returns:
// 2 = skipped waiting for another frame
// 1 = good result and more frames exist
// 0 = no more frames exist, a frame may or may not have been played: use getLastError() and look for
// GIF_SUCCESS to know if a frame was played -1 = error
int Gif::playFrame(int x, int y, bool bSync) {
    if (bSync && ((millis() - lTime) >= *delayMilliseconds)) {
        lTime = millis();
        gifPosition.x = x;
        gifPosition.y = y;
        return gif->playFrame(false, delayMilliseconds, &gifPosition);
    }

    return 2;
}

int Gif::getLastError() { return gif->getLastError(); }

/*
 * playDurationMs:
 *  -1 : Play the GIF in an infinite loop
 *  0  : Play the GIF once
 * >0  : Play the GIF for the specified duration in milliseconds
 *       (e.g., 1000 = play for 1 second)
 */
bool showGif(FS *fs, const char *filename, int x, int y, bool center, int playDurationMs) {
    if (!fs->exists(filename)) return false;

    Gif gif;
    bool success = gif.openGIF(fs, filename);
    if (!success) { return false; }

    if (center) {
        x = x + (tftWidth - gif.getCanvasWidth()) / 2;
        y = y + (tftHeight - gif.getCanvasHeight()) / 2;
    }

    int result = 0;
    long timeStart = millis();
    do {
        result = gif.playFrame(x, y);
        if (result == -1) log_e("GIF playFrame error: %d\n", gif.getLastError());

        if (check(AnyKeyPress)) break;

        if (playDurationMs > 0 && (millis() - timeStart) > playDurationMs) break;
        if (playDurationMs == 0 && result == 0) break;
    } while (result >= 0);

    return true;
}
#endif
/***************************************************************************************
** Function name: getComplementaryColor2
** Description:   Get simple complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor2(uint16_t color) {
    int r = 31 - ((color >> 11) & 0x1F);
    int g = 63 - ((color >> 5) & 0x3F);
    int b = 31 - (color & 0x1F);
    return (r << 11) | (g << 5) | b;
}
/***************************************************************************************
** Function name: getComplementaryColor
** Description:   Get complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor(uint16_t color) {
    double r = ((color >> 11) & 0x1F) / 31.0;
    double g = ((color >> 5) & 0x3F) / 63.0;
    double b = (color & 0x1F) / 31.0;

    double cmax = fmax(r, fmax(g, b));
    double cmin = fmin(r, fmin(g, b));
    double delta = cmax - cmin;

    double hue = 0.0;
    if (delta == 0) hue = 0.0;
    else if (cmax == r) hue = 60 * fmod((g - b) / delta, 6);
    else if (cmax == g) hue = 60 * ((b - r) / delta + 2);
    else hue = 60 * ((r - g) / delta + 4);

    if (hue < 0) hue += 360;

    double lightness = (cmax + cmin) / 2;
    double saturation = (delta == 0) ? 0 : delta / (1 - std::abs(2 * lightness - 1));

    double compHue = fmod(hue + 180, 360);

    double c = (1 - std::abs(2 * lightness - 1)) * saturation;
    double x = c * (1 - std::abs(fmod(compHue / 60, 2) - 1));
    double m = lightness - c / 2;

    double compR = 0, compG = 0, compB = 0;
    if (compHue >= 0 && compHue < 60) {
        compR = c;
        compG = x;
    } else if (compHue >= 60 && compHue < 120) {
        compR = x;
        compG = c;
    } else if (compHue >= 120 && compHue < 180) {
        compG = c;
        compB = x;
    } else if (compHue >= 180 && compHue < 240) {
        compG = x;
        compB = c;
    } else if (compHue >= 240 && compHue < 300) {
        compB = c;
        compR = x;
    } else {
        compB = x;
        compR = c;
    }

    uint16_t compl_color = uint8_t(compR * 31) << 11 | uint8_t(compG * 63) << 5 | uint8_t(compB * 31);

    // change black color
    if (compl_color == 0) compl_color = color - 0x1111;

    return compl_color;
}

/***************************************************************************************
** Function name: getColorVariation
** Description:   Get a variation of color in RGB565 format
***************************************************************************************/
uint16_t getColorVariation(uint16_t color, int delta, int direction) {
    uint8_t r = ((color >> 11) & 0x1F);
    uint8_t g = ((color >> 5) & 0x3F);
    uint8_t b = (color & 0x1F);

    float brightness = 0.299 * r / 31 + 0.587 * g / 63 + 0.114 * b / 31;

    if (direction < 0 || (direction == 0 && brightness >= 0.5)) {
        r = max(0, r - delta);
        g = max(0, g - 2 * delta);
        b = max(0, b - delta);
    } else {
        r = min(31, r + delta);
        g = min(63, g + 2 * delta);
        b = min(31, b + delta);
    }

    uint16_t compl_color = r << 11 | g << 5 | b;

    return compl_color;
}

// Draw BITMAP files
// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(fs::File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}
bool drawBmp(FS &fs, String filename, int x, int y, bool center) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    uint32_t startTime = millis();

    File bmpFS;

    // Open requested file on SD card
    bmpFS = fs.open(filename, "r");

    if (!bmpFS) {
        Serial.print("File not found");
        goto ERROR;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    if (read16(bmpFS) == 0x4D42) {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);
        if (center) {
            x = x + (tftWidth - w) / 2;
            y = y + (tftHeight - h) / 2;
        }

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)) {
            y += h - 1;

            bool oldSwapBytes = tft.getSwapBytes();
            tft.setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++) {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16-bit colours
                for (uint16_t col = 0; col < w; col++) {
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                tft.drawPixel(
                    0, 0, 0
                ); // shared TFT_Spi devices struggle to work, need call a line first sometimes
                tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            tft.setSwapBytes(oldSwapBytes);
            Serial.print("BMP Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        } else {
            goto ERROR;
        }
    } else {
    ERROR:
        Serial.println("BMP format not recognized.");
        bmpFS.close();
        return false;
    }
    bmpFS.close();
    return true;
}

bool drawImg(FS &fs, String filename, int x, int y, bool center, int playDurationMs) {
    String ext = filename.substring(filename.lastIndexOf('.'));
    ext.toLowerCase();
    uint8_t fls = 2;         // 2 for Little FS
    if (&fs == &SD) fls = 0; // 0 for SD
    tft.imageToBin(fls, filename, x, y, center, playDurationMs);
    if (ext.endsWith("jpg")) return showJpeg(fs, filename, x, y, center);
    else if (ext.endsWith("bmp")) return drawBmp(fs, filename, x, y, center);
    else if (ext.endsWith("png")) return drawPNG(fs, filename, x, y, center);

#if !defined(LITE_VERSION)

    else if (ext.endsWith("gif")) return showGif(&fs, filename.c_str(), x, y, center, playDurationMs);
#endif
    else log_e("Image not supported");

    return false;
}

#if !defined(LITE_VERSION)
/// Draw PNG files

#include <PNGdec.h>
#if TFT_WIDTH > TFT_HEIGHT
#define MAX_IMAGE_WIDTH TFT_WIDTH
#else
#define MAX_IMAGE_WIDTH TFT_HEIGHT
#endif
PNG *png = nullptr;
// Optional pointer to write decoded lines into a cached BIN file
static File *pngBinOut = nullptr;
static bool pngCacheOnly = false;
// Optionally use heap capabilities on ESP32 to pick the best memory region for the decoder
#if defined(ESP32)
#include <esp_heap_caps.h>
#endif
// Functions to access a file on the SD card
File myfile;
FS *_fs;

void *myOpen(const char *filename, int32_t *size) {
    // Serial.printf("Attempting to open %s\n", filename);
    myfile = _fs->open(filename);
    *size = myfile.size();
    return &myfile;
}
void myClose(void *handle) {
    if (myfile) myfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
    if (!myfile) return 0;
    return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
    if (!myfile) return 0;
    return myfile.seek(position);
}
// Function to draw pixels to the display
int16_t xpos = 0;
int16_t ypos = 0;
int PNGDraw(PNGDRAW *pDraw) {
    uint16_t usPixels[MAX_IMAGE_WIDTH];
    // static uint16_t dmaBuffer[MAX_IMAGE_WIDTH]; // static so buffer persists after fn exit
    uint8_t r = ((uint16_t)bruceConfig.bgColor & 0xF800) >> 8;
    uint8_t g = ((uint16_t)bruceConfig.bgColor & 0x07E0) >> 3;
    uint8_t b = ((uint16_t)bruceConfig.bgColor & 0x001F) << 3;
    png->getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, b << 16 | g << 8 | r);
    if (!pngCacheOnly) {
        tft.drawPixel(0, 0, 0);
        tft.drawPixel(0, 0, 0);
        tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, usPixels);
    }
    if (pngBinOut) { pngBinOut->write((uint8_t *)usPixels, pDraw->iWidth * sizeof(uint16_t)); }
    return 1;
}

// Build a cache path alongside the PNG: <dir>/tmp/<basename>.bin
static String buildPngBinPath(const String &pngPath) {
    int slash = pngPath.lastIndexOf('/');
    String dir = (slash >= 0) ? pngPath.substring(0, slash) : "";
    String name = pngPath.substring(slash + 1);
    int dot = name.lastIndexOf('.');
    if (dot > 0) name = name.substring(0, dot);

    String tmpDir = dir.length() ? dir + "/tmp" : "/tmp";
    if (!tmpDir.startsWith("/")) tmpDir = "/" + tmpDir;

    return tmpDir + "/" + name + ".bin";
}

static bool ensureTmpDir(FS &fs, const String &binPath) {
    int slash = binPath.lastIndexOf('/');
    if (slash < 0) return false;
    String dir = binPath.substring(0, slash);
    if (fs.exists(dir)) return true;
    return fs.mkdir(dir);
}

// Render a previously cached BIN (RGB565 LE with 2-byte width/height header)
static bool drawPngBin(FS &fs, const String &binPath, int x, int y, bool center) {
    File f = fs.open(binPath, FILE_READ);
    if (!f) return false;

    uint16_t w = 0, h = 0;
    if (f.read((uint8_t *)&w, sizeof(uint16_t)) != sizeof(uint16_t) ||
        f.read((uint8_t *)&h, sizeof(uint16_t)) != sizeof(uint16_t)) {
        f.close();
        return false;
    }

    if (center) {
        x = x + (tftWidth - w) / 2;
        y = y + (tftHeight - h) / 2;
    }

    if (x >= tft.width() || y >= tft.height()) {
        f.close();
        return false;
    }

    std::unique_ptr<uint16_t[]> line(new (std::nothrow) uint16_t[w]);
    if (!line) {
        f.close();
        return false;
    }

    size_t rowBytes = w * sizeof(uint16_t);
    for (uint16_t row = 0; row < h; ++row) {
        if (f.read((uint8_t *)line.get(), rowBytes) != rowBytes) {
            f.close();
            return false;
        }
        tft.pushImage(x, y + row, w, 1, line.get());
    }

    f.close();
    return true;
}

bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    _fs = &fs;
    uint32_t dt = millis();

    String binPath = buildPngBinPath(filename);
    if (fs.exists(binPath)) {
        if (pngCacheOnly) return true; // cache already ready
        if (drawPngBin(fs, binPath, x, y, center)) return true;
        fs.remove(binPath); // stale cache, fall back to decode
    }

    // Allocate decoder only while drawing, then release to keep RAM available for Wi-Fi/AP usage
#if defined(ESP32)
    bool usedHeapCaps = true;
    void *mem = psramFound() ? heap_caps_malloc(sizeof(PNG), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                             : heap_caps_malloc(sizeof(PNG), MALLOC_CAP_8BIT);
    if (!mem) {
        mem = malloc(sizeof(PNG));
        usedHeapCaps = false;
    }
#else
    void *mem = malloc(sizeof(PNG));
#endif
#if !defined(ESP32)
    bool usedHeapCaps = false;
#endif

    if (!mem) {
        Serial.println("Fail alloc PNG!");
        bruceConfig.theme.label = true;
        return false;
    }

    png = new (mem) PNG();
    int16_t rc = png->open(filename.c_str(), myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        // Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png->getWidth(),
        // png->getHeight(), png->getBpp(), png->getPixelType());

        File binFile;
        if (ensureTmpDir(fs, binPath)) {
            binFile = fs.open(binPath, FILE_WRITE);
            if (binFile) {
                uint16_t w = png->getWidth();
                uint16_t h = png->getHeight();
                binFile.write((uint8_t *)&w, sizeof(uint16_t));
                binFile.write((uint8_t *)&h, sizeof(uint16_t));
                pngBinOut = &binFile;
            }
        }

        if (center) {
            xpos = x + (tftWidth - png->getWidth()) / 2;
            ypos = y + (tftHeight - png->getHeight()) / 2;
        }

        if (png->getWidth() > MAX_IMAGE_WIDTH) {
            Serial.println("Image too wide for allocated line buffer size!");
        } else {
            rc = png->decode(NULL, 0);
            png->close();
        }

        if (pngBinOut) {
            pngBinOut->close();
            pngBinOut = nullptr;
        } else {
            if (fs.exists(binPath) && rc != PNG_SUCCESS) fs.remove(binPath);
        }
        if (rc != PNG_SUCCESS && fs.exists(binPath)) { fs.remove(binPath); }

        // How long did rendering take...
        Serial.print("PNG Loaded in ");
        Serial.print(millis() - dt);
        Serial.println("ms");
    } else {
        // Decode/open failed, ensure no stale cache
        if (fs.exists(binPath)) fs.remove(binPath);
    }

    // Destroy placement-new object and free memory so RAM is available after rendering
    png->~PNG();
#if defined(ESP32)
    if (usedHeapCaps) heap_caps_free(mem);
    else free(mem);
#else
    free(mem);
#endif
    png = nullptr;

    return rc == PNG_SUCCESS;
}

// Prepare (or verify) the cached BIN for a PNG without rendering it on screen
bool preparePngBin(FS &fs, String filename) {
    bool previous = pngCacheOnly;
    pngCacheOnly = true;
    bool ok = drawPNG(fs, filename, 0, 0, false);
    pngCacheOnly = previous;
    return ok;
}
#else
bool preparePngBin(FS &fs, String filename) {
    log_w("PNG: Not supported in this version");
    return true;
}
bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    log_w("PNG: Not supported in this version");
    return false;
}
#endif
