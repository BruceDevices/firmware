#include "ducky_typer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#define DEF_DELAY 100

uint8_t _Ask_for_restart = 0;

#if !defined(USB_as_HID)
HardwareSerial mySerial(1);
#endif

HIDInterface *hid_usb = nullptr;
HIDInterface *hid_ble = nullptr;

enum DuckyCommandType {
    DuckyCommandType_Unknown,
    DuckyCommandType_Cmd,
    DuckyCommandType_Print,
    DuckyCommandType_Delay,
    DuckyCommandType_Comment,
    DuckyCommandType_Loop,
    DuckyCommandType_Combination
};

struct DuckyCommand {
    const char *command;
    char key;
    DuckyCommandType type;
};

struct DuckyCombination {
    const char *command;
    char key1;
    char key2;
    char key3;
};
const DuckyCombination duckyComb[]{
    {"CTRL-ALT",       KEY_LEFT_CTRL, KEY_LEFT_ALT,   0             },
    {"CTRL-SHIFT",     KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 0             },
    {"CTRL-GUI",       KEY_LEFT_CTRL, KEY_LEFT_GUI,   0             },
    {"CTRL-ESCAPE",    KEY_LEFT_CTRL, KEY_ESC,        0             },
    {"ALT-SHIFT",      KEY_LEFT_ALT,  KEY_LEFT_SHIFT, 0             },
    {"ALT-GUI",        KEY_LEFT_ALT,  KEY_LEFT_GUI,   0             },
    {"GUI-SHIFT",      KEY_LEFT_GUI,  KEY_LEFT_SHIFT, 0             },
    {"GUI-SPACE",      KEY_LEFT_GUI,  KEY_SPACE,      0             },
    {"CTRL-ALT-SHIFT", KEY_LEFT_CTRL, KEY_LEFT_ALT,   KEY_LEFT_SHIFT},
    {"CTRL-ALT-GUI",   KEY_LEFT_CTRL, KEY_LEFT_ALT,   KEY_LEFT_GUI  },
    {"ALT-SHIFT-GUI",  KEY_LEFT_ALT,  KEY_LEFT_SHIFT, KEY_LEFT_GUI  },
    {"CTRL-SHIFT-GUI", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_GUI  }
};

const DuckyCommand duckyCmds[]{
    {"STRING",         0,                DuckyCommandType_Print      },
    {"STRINGLN",       0,                DuckyCommandType_Print      },
    {"REM",            0,                DuckyCommandType_Comment    },
    {"DELAY",          0,                DuckyCommandType_Delay      },
    {"DEFAULTDELAY",   DEF_DELAY,        DuckyCommandType_Delay      },
    {"REPEAT",         0,                DuckyCommandType_Loop       },
    {"CTRL-ALT",       0,                DuckyCommandType_Combination},
    {"CTRL-SHIFT",     0,                DuckyCommandType_Combination},
    {"CTRL-GUI",       0,                DuckyCommandType_Combination},
    {"CTRL-ESCAPE",    0,                DuckyCommandType_Combination},
    {"ALT-SHIFT",      0,                DuckyCommandType_Combination},
    {"ALT-GUI",        0,                DuckyCommandType_Combination},
    {"GUI-SHIFT",      0,                DuckyCommandType_Combination},
    {"GUI-SPACE",      0,                DuckyCommandType_Combination},
    {"CTRL-ALT-SHIFT", 0,                DuckyCommandType_Combination},
    {"CTRL-ALT-GUI",   0,                DuckyCommandType_Combination},
    {"ALT-SHIFT-GUI",  0,                DuckyCommandType_Combination},
    {"CTRL-SHIFT-GUI", 0,                DuckyCommandType_Combination},
    {"BACKSPACE",      KEYBACKSPACE,     DuckyCommandType_Cmd        },
    {"DELETE",         KEY_DELETE,       DuckyCommandType_Cmd        },
    {"ALT",            KEY_LEFT_ALT,     DuckyCommandType_Cmd        },
    {"CTRL",           KEY_LEFT_CTRL,    DuckyCommandType_Cmd        },
    {"GUI",            KEY_LEFT_GUI,     DuckyCommandType_Cmd        },
    {"SHIFT",          KEY_LEFT_SHIFT,   DuckyCommandType_Cmd        },
    {"ESCAPE",         KEY_ESC,          DuckyCommandType_Cmd        },
    {"TAB",            KEYTAB,           DuckyCommandType_Cmd        },
    {"ENTER",          KEY_RETURN,       DuckyCommandType_Cmd        },
    {"DOWNARROW",      KEY_DOWN_ARROW,   DuckyCommandType_Cmd        },
    {"DOWN",           KEY_DOWN_ARROW,   DuckyCommandType_Cmd        },
    {"LEFTARROW",      KEY_LEFT_ARROW,   DuckyCommandType_Cmd        },
    {"LEFT",           KEY_LEFT_ARROW,   DuckyCommandType_Cmd        },
    {"RIGHTARROW",     KEY_RIGHT_ARROW,  DuckyCommandType_Cmd        },
    {"RIGHT",          KEY_RIGHT_ARROW,  DuckyCommandType_Cmd        },
    {"UPARROW",        KEY_UP_ARROW,     DuckyCommandType_Cmd        },
    {"UP",             KEY_UP_ARROW,     DuckyCommandType_Cmd        },
    {"BREAK",          KEY_PAUSE,        DuckyCommandType_Cmd        },
    {"CAPSLOCK",       KEY_CAPS_LOCK,    DuckyCommandType_Cmd        },
    {"PAUSE",          KEY_PAUSE,        DuckyCommandType_Cmd        },
    {"END",            KEY_END,          DuckyCommandType_Cmd        },
    {"HOME",           KEY_HOME,         DuckyCommandType_Cmd        },
    {"INSERT",         KEY_INSERT,       DuckyCommandType_Cmd        },
    {"NUMLOCK",        LED_NUMLOCK,      DuckyCommandType_Cmd        },
    {"PAGEUP",         KEY_PAGE_UP,      DuckyCommandType_Cmd        },
    {"PAGEDOWN",       KEY_PAGE_DOWN,    DuckyCommandType_Cmd        },
    {"PRINTSCREEN",    KEY_PRINT_SCREEN, DuckyCommandType_Cmd        },
    {"SCROLLOCK",      KEY_SCROLL_LOCK,  DuckyCommandType_Cmd        },
    {"MENU",           KEY_MENU,         DuckyCommandType_Cmd        },
    {"F1",             KEY_F1,           DuckyCommandType_Cmd        },
    {"F2",             KEY_F2,           DuckyCommandType_Cmd        },
    {"F3",             KEY_F3,           DuckyCommandType_Cmd        },
    {"F4",             KEY_F4,           DuckyCommandType_Cmd        },
    {"F5",             KEY_F5,           DuckyCommandType_Cmd        },
    {"F6",             KEY_F6,           DuckyCommandType_Cmd        },
    {"F7",             KEY_F7,           DuckyCommandType_Cmd        },
    {"F8",             KEY_F8,           DuckyCommandType_Cmd        },
    {"F9",             KEY_F9,           DuckyCommandType_Cmd        },
    {"F10",            KEY_F10,          DuckyCommandType_Cmd        },
    {"F11",            KEY_F11,          DuckyCommandType_Cmd        },
    {"F12",            KEY_F12,          DuckyCommandType_Cmd        },
    {"SPACE",          KEY_SPACE,        DuckyCommandType_Cmd        }
};

const uint8_t *keyboardLayouts[] = {
    KeyboardLayout_en_US, // 0
    KeyboardLayout_da_DK, // 1
    KeyboardLayout_en_UK, // 2
    KeyboardLayout_fr_FR, // 3
    KeyboardLayout_de_DE, // 4
    KeyboardLayout_hu_HU, // 5
    KeyboardLayout_it_IT, // 6
    KeyboardLayout_en_US, // 7
    KeyboardLayout_pt_BR, // 8
    KeyboardLayout_pt_PT, // 9
    KeyboardLayout_si_SI, // 10
    KeyboardLayout_es_ES, // 11
    KeyboardLayout_sv_SE, // 12
    KeyboardLayout_tr_TR  // 13
};

void ducky_startKb(HIDInterface *&hid, bool ble) {
    Serial.printf("\nducky_startKb before hid==null: BLE: %d\n", ble);
    if (hid == nullptr) {
        Serial.printf("ducky_startKb after hid==null: BLE: %d\n", ble);
        if (ble) {
            // _Ask_for_restart change to 2 when use Disconnect option in BLE menu
            if (_Ask_for_restart == 2) {
                displayError("Restart your Device");
                returnToMenu = true;
            }
            hid = new BleKeyboard(bruceConfigPins.bleName, "BruceFW", 100);
        } else {
#if defined(USB_as_HID)
            hid = new USBHIDKeyboard();
            USB.begin();
#else
            mySerial.begin(CH9329_DEFAULT_BAUDRATE, SERIAL_8N1, BAD_RX, BAD_TX);
            delay(100);
            hid = new CH9329_Keyboard_();
#endif
        }
    }
    if (ble) {
        if (hid->isConnected()) {
            // If connected as media controller and switch to BadBLE, changes the layout
            hid->setLayout(keyboardLayouts[bruceConfig.badUSBBLEKeyboardLayout]);
            hid->setDelay(bruceConfig.badUSBBLEKeyDelay);
            return;
        }
        if (!_Ask_for_restart) _Ask_for_restart = 1; // arm the flag
        hid->begin(keyboardLayouts[bruceConfig.badUSBBLEKeyboardLayout]);
        hid->setDelay(bruceConfig.badUSBBLEKeyDelay);
    } else {
#if defined(USB_as_HID)
        hid->begin(keyboardLayouts[bruceConfig.badUSBBLEKeyboardLayout]);
        hid->setDelay(bruceConfig.badUSBBLEKeyDelay);
#else
        mySerial.begin(CH9329_DEFAULT_BAUDRATE, SERIAL_8N1, BAD_RX, BAD_TX);
        delay(100);
        hid->begin(mySerial, keyboardLayouts[bruceConfig.badUSBBLEKeyboardLayout]);
        hid->setDelay(bruceConfig.badUSBBLEKeyDelay);
#endif
    }
}

// Start badUSBBLE or badBLE ducky runner
void ducky_setup(HIDInterface *&hid, bool ble) {
    Serial.println("Ducky typer begin");
    tft.fillScreen(bruceConfig.bgColor);

    if (ble && _Ask_for_restart == 2) {
        displayError("Restart your Device");
        returnToMenu = true;
        return;
    }
    FS *fs = nullptr;
    bool first_time = true;
NewScript:
    tft.fillScreen(bruceConfig.bgColor);
    String bad_script = "";
    options = {};

    if (setupSdCard()) {
        options.push_back({"SD Card", [&]() { fs = &SD; }});
    }
    options.push_back({"LittleFS", [&]() { fs = &LittleFS; }});
    options.push_back({"Main Menu", [&]() { fs = nullptr; }});

    loopOptions(options);

    if (fs != nullptr) {
        bad_script = loopSD(*fs, true);
        if (bad_script == "") {
            displayWarning("Canceled", true);
            returnToMenu = true;
            goto EXIT;
        }
        tft.fillScreen(bruceConfig.bgColor);
        if (first_time) {
            ducky_startKb(hid, ble);
            if (returnToMenu) goto EXIT; // make sure to free the hid object before exiting
            first_time = false;
            if (!ble) {
#if !defined(USB_as_HID)
                mySerial.write(0x00);
                while (mySerial.available() <= 0) {
                    if (mySerial.available() <= 0) {
                        displayTextLine("CH9329 -> USB");
                        delay(200);
                        mySerial.write(0x00);
                    } else break;
                    if (check(EscPress)) {
                        displayError("CH9329 not found"); // Cancel run
                        delay(500);
                        goto EXIT;
                    }
                }
#endif
                displayTextLine("Preparing");
                delay(2000); // Time to Computer or device recognize the USB HID
            } else {
                displayTextLine("Waiting Victim");
                while (!hid->isConnected() && !check(EscPress));
                if (hid->isConnected()) {
                    BLEConnected = true;
                    displayTextLine("Preparing");
                    delay(1000);
                } else {
                    displayWarning("Canceled", true);
                    goto EXIT;
                }
            }
        }
        displayWarning(String(BTN_ALIAS) + " to deploy", true);
        delay(200);
        key_input(*fs, bad_script, hid);

        displayTextLine("Payload Sent", true);

        if (returnToMenu) goto EXIT;
        // Try to run a new script on the same device
        goto NewScript;
    } else displayWarning("Canceled", true);
EXIT:
    if (!ble) {
        delete hid; // Keep the hid object alive for BLE
        hid = nullptr;
#if !defined(USB_as_HID)
        mySerial.end();       // Stops UART Serial as HID
        Serial.begin(115200); // Force restart of Serial, just in case....
#endif
    }
    returnToMenu = true;
}
// Parses a file to run in the badUSBBLE
void key_input(FS fs, String bad_script, HIDInterface *_hid) {
    if (!fs.exists(bad_script) || bad_script == "") return;
    File payloadFile = fs.open(bad_script, "r");
    if (!payloadFile) return;
    tft.setCursor(0, 40);
    tft.println("from file!");
    String lineContent = "";
    String Command = "";
    char Cmd[15];
    String Argument = "";
    String RepeatTmp = "";
    char ArgChar = '\0';
    bool ArgIsCmd; // Verifies if the Argument is DELETE, TAB or F1-F12

    _hid->releaseAll();
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.fillScreen(bruceConfig.bgColor);

    while (payloadFile.available()) {
        previousMillis = millis(); // resets DimScreen
        if (check(SelPress)) {
            while (check(SelPress)); // hold the code in this position until release the btn
            options = {
                {"Continue", yield},
            };
            addOptionToMainMenu();
            loopOptions(options);

            if (returnToMenu) break;
            tft.setTextSize(FP);
        }
        // CRLF is a combination of two control characters: the "Carriage Return" represented by
        // the character "\r" and the "Line Feed" represented by the character "\n".
        lineContent = payloadFile.readStringUntil('\n');
        if (lineContent.endsWith("\r")) lineContent.remove(lineContent.length() - 1);

        RepeatTmp = lineContent.substring(0, lineContent.indexOf(' '));
        RepeatTmp = RepeatTmp.c_str();
        if (RepeatTmp == "REPEAT") {
            if (lineContent.indexOf(' ') > 0) {
                // how many times it will repeat, using .toInt() conversion;
                RepeatTmp = lineContent.substring(lineContent.indexOf(' ') + 1);
                if (RepeatTmp.toInt() == 0) {
                    RepeatTmp = "1";
                    tft.setTextColor(ALCOLOR);
                    tft.println("REPEAT argument NaN, repeating once");
                }
            } else {
                RepeatTmp = "1";
                tft.setTextColor(ALCOLOR);
                tft.println("REPEAT without argument, repeating once");
            }
        } else {
            Command = lineContent.substring(0, lineContent.indexOf(' ')); // get the Command
            strcpy(Cmd, Command.c_str());                                 // get the cmd
            if (lineContent.indexOf(' ') > 0)
                Argument = lineContent.substring(lineContent.indexOf(' ') + 1); // get the argument
            else Argument = "";
            RepeatTmp = "1";
        }
        uint16_t i;
        ArgIsCmd = false;
        Argument = Argument.c_str();
        ArgChar = Argument.charAt(0);
        for (i = 0; i < RepeatTmp.toInt(); i++) {
            DuckyCommand *ArgCmd = nullptr;
            DuckyCommand *PriCmd = nullptr;
            ArgIsCmd = false;
            for (auto cmds : duckyCmds) {
                if (strcmp(Cmd, cmds.command) == 0) {
                    PriCmd = &cmds;
                    // STRING and STRINGLN are processed here
                    if (cmds.type == DuckyCommandType_Print) {
                        _hid->print(Argument);
                        if (strcmp(cmds.command, "STRINGLN") == 0) _hid->println();
                        break;
                    }
                    // DELAY and DEFAULTDELAY are processed here
                    else if (cmds.type == DuckyCommandType_Delay) {
                        if ((int)cmds.key > 0) delay(DEF_DELAY); // Default delay is 100ms
                        else if (Argument.toInt() > 0) delay(Argument.toInt());
                        else delay(DEF_DELAY);
                        break;
                    }
                    // Comment line is porocessed Here
                    else if (cmds.type == DuckyCommandType_Comment) {
                        yield(); // do nothing, just wait for the next line
                        break;
                    }
                    // Normal commands are processed here
                    else if (cmds.type == DuckyCommandType_Cmd) {
                        _hid->press(cmds.key);
                        ArgIsCmd = true;
                    }
                    // Combinations are processed here
                    else if (cmds.type == DuckyCommandType_Combination) {
                        for (auto comb : duckyComb) {
                            if (strcmp(Cmd, comb.command) == 0) {
                                _hid->press(comb.key1);
                                _hid->press(comb.key2);
                                if (comb.key3 != 0) _hid->press(comb.key3);
                                ArgIsCmd = true;
                            }
                        }
                    }
                }
                // check if the Argument contains a command
                if (strcmp(Argument.c_str(), cmds.command) == 0) { ArgCmd = &cmds; }
            }

            if (ArgCmd != nullptr && PriCmd != nullptr) {
                if (ArgCmd->type == DuckyCommandType_Cmd) { _hid->press(ArgCmd->key); }
            } else if (ArgIsCmd && PriCmd != nullptr) {
                if (ArgChar != '\0') _hid->press(ArgChar);
            }
            _hid->releaseAll();

            if (PriCmd == nullptr) {
                tft.setTextColor(ALCOLOR);
                tft.print(Command);
                tft.println(" -> Not Supported, running as STRINGLN");
                if (Argument != "") {
                    _hid->println(Command + " " + Argument);
                } else {
                    _hid->println(Command);
                }
            } else {
                tft.setTextColor(bruceConfig.priColor);
                tft.print(Command);
            }
            if (Argument.length() > 0) {
                tft.setTextColor(TFT_WHITE);
                tft.println(Argument);
            } else tft.println();
        }
    }
    tft.setTextSize(FM);
    payloadFile.close();
    _hid->releaseAll();
}

// Sends a simple command
void key_input_from_string(String text) {
    ducky_startKb(hid_usb, false);

    hid_usb->print(text.c_str()); // buggy with some special chars

    delete hid_usb;
    hid_usb = nullptr;
#if !defined(USB_as_HID)
    mySerial.end();
#endif
}
#ifndef KB_HID_EXIT_MSG
#define KB_HID_EXIT_MSG "Exit"
#endif
// Use device as a keyboard (USB or BLE)
void ducky_keyboard(HIDInterface *&hid, bool ble) {
    String _mymsg = "";
    keyStroke key;
    long debounce = millis();
    ducky_startKb(hid, ble);
    if (returnToMenu) return;

    if (ble) {
        displayTextLine("Waiting Victim");
        while (!hid->isConnected() && !check(EscPress));
        if (hid->isConnected()) {
            BLEConnected = true;
        } else {
            displayWarning("Canceled", true);
            goto EXIT;
        }
    } else {
        // send a key to start communication
        hid->press(KEY_LEFT_ALT);
        hid->releaseAll();
    }

    drawMainBorder();
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor);
    tft.drawString("Keyboard Started", tftWidth / 2, tftHeight / 2);

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    drawMainBorder();
    tft.setCursor(10, 28);
    if (ble) tft.println("BLE Keyboard:");
    else tft.println("Usb Keyboard:");
    tft.drawCentreString("> " + String(KB_HID_EXIT_MSG) + " <", tftWidth / 2, tftHeight - 20, 1);
    tft.setTextSize(FM);

    while (1) {
#if defined(HAS_KEYBOARD)
        key = _getKeyPress();
        if (key.pressed && (millis() - debounce > 200)) {
            if (key.alt) hid->press(KEY_LEFT_ALT);
            if (key.ctrl) hid->press(KEY_LEFT_CTRL);
            if (key.gui) hid->press(KEY_LEFT_GUI);
            if (key.enter) hid->println();
            else if (key.del) hid->press(KEYBACKSPACE);
            else {
                for (char k : key.word) { hid->press(k); }
                for (auto k : key.modifier_keys) { hid->press(k); }
            }
            if (key.fn && key.exit_key) break;

            hid->releaseAll();

            // only text for tft
            String keyStr = "";
            for (auto i : key.word) {
                if (keyStr != "") {
                    keyStr = keyStr + "+" + i;
                } else {
                    keyStr += i;
                }
            }

            if (keyStr.length() > 0) {
                drawMainBorder(false);
                if (_mymsg.length() > keyStr.length())
                    tft.drawCentreString(
                        "                                  ", tftWidth / 2, tftHeight / 2, 1
                    ); // clears screen
                tft.drawCentreString("Pressed: " + keyStr, tftWidth / 2, tftHeight / 2, 1);
                _mymsg = keyStr;
            }
            debounce = millis();
        }
#else
        hid->releaseAll();
        static int inx = 0;
        String str = "";
        const DuckyCommand *cmd = nullptr;
        options = {};
        for (auto &cmds : duckyCmds) {
            auto &cmds_cpy = cmds;
            if (cmds_cpy.type != DuckyCommandType_Delay && cmds_cpy.type != DuckyCommandType_Comment &&
                cmds_cpy.type != DuckyCommandType_Loop) {
                options.push_back({cmds.command, [&]() { cmd = &cmds_cpy; }});
            }
        }
        addOptionToMainMenu();
        inx = loopOptions(options, inx);
        options.clear();
        if (returnToMenu || cmd == nullptr) break;
        if (cmd->type == DuckyCommandType_Print) {
            str = keyboard("", 76, "Type your message:");
            if (str.length() > 0) {
                hid->print(str.c_str());
                if (strcmp(cmd->command, "STRINGLN") == 0) hid->println();
            }
        } else if (cmd->type == DuckyCommandType_Cmd) {
            str = keyboard("", 1, "Type a character:");
            hid->press(cmd->key);
            if (str.length() > 0) { hid->press(str.c_str()[0]); }
        } else if (cmd->type == DuckyCommandType_Combination) {
            for (auto comb : duckyComb) {
                if (strcmp(cmd->command, comb.command) == 0) {
                    str = keyboard("", 1, "Type a character:");
                    hid->press(comb.key1);
                    hid->press(comb.key2);
                    if (comb.key3 != 0) hid->press(comb.key3);
                    if (str.length() > 0) { hid->press(str.c_str()[0]); }
                }
            }
        }
        hid->releaseAll();
#endif
    }
EXIT:
    if (!ble) {
        delete hid; // Keep the hid object alive for BLE
        hid = nullptr;
#if !defined(USB_as_HID)
        mySerial.end();       // Stops UART Serial as HID
        Serial.begin(115200); // Force restart of Serial, just in case....
#endif
    }
}

// Send media commands through BLE or USB HID
void MediaCommands(HIDInterface *hid, bool ble) {
    if (_Ask_for_restart == 2) return;
    _Ask_for_restart = 1; // arm the flag

    ducky_startKb(hid, true);

    displayTextLine("Pairing...");

    while (!hid->isConnected() && !check(EscPress));

    if (hid->isConnected()) {
        BLEConnected = true;
        drawMainBorder();
        int index = 0;

    reMenu:
        options = {
            {"ScreenShot", [=]() { hid->press(KEY_PRINT_SCREEN); }        },
            {"Play/Pause", [=]() { hid->press(KEY_MEDIA_PLAY_PAUSE); }    },
            {"Stop",       [=]() { hid->press(KEY_MEDIA_STOP); }          },
            {"Next Track", [=]() { hid->press(KEY_MEDIA_NEXT_TRACK); }    },
            {"Prev Track", [=]() { hid->press(KEY_MEDIA_PREVIOUS_TRACK); }},
            {"Volume +",   [=]() { hid->press(KEY_MEDIA_VOLUME_UP); }     },
            {"Volume -",   [=]() { hid->press(KEY_MEDIA_VOLUME_DOWN); }   },
            {"Hold Vol +",
             [=]() {
                 hid->press(KEY_MEDIA_VOLUME_UP);
                 delay(1000);
                 hid->releaseAll();
             }                                                            },
            {"Mute",       [=]() { hid->press(KEY_MEDIA_MUTE); }          },
        };
        addOptionToMainMenu();
        index = loopOptions(options, index);
        hid->releaseAll();
        if (!returnToMenu) goto reMenu;
    }
    returnToMenu = true;
}
