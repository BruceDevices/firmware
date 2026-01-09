#include "util_commands.h"
#include "core/main_menu.h"
#include "core/sd_functions.h"
#include "core/utils.h" // to return optionsJSON
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h" //to return MAC addr
#include "modules/badusb_ble/ducky_typer.h"
#include <Wire.h>
#include <globals.h>

uint32_t uptimeCallback(cmd *c) {
    // https://github.com/espressif/arduino-esp32/blob/66c9c0b1a6a36b85d27cdac0fb52098368de1a09/libraries/WebServer/examples/AdvancedWebServer/AdvancedWebServer.ino#L64

    int sec = millis() / 1000;
    int hr = sec / 3600;
    int min = (sec / 60) % 60;
    sec = sec % 60;
    char temp[400];
    snprintf(temp, 400, "Uptime: %02d:%02d:%02d", hr, min, sec);
    serialDevice->println(temp);
    return true;
}

uint32_t dateCallback(cmd *c) {
    if (!clock_set) {
        serialDevice->println("Clock not set");
        return false;
    }

    serialDevice->print("Current time: ");
#if !defined(HAS_RTC)
    serialDevice->println(rtc.getDateTime());
    // serialDevice->println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
#else
    _rtc.begin();
    _rtc.GetTime(&_time);
    _rtc.GetDate(&_date);
    char stimeStr[100] = {0};
    snprintf(
        stimeStr,
        sizeof(stimeStr),
        "%02d %02d %04d %02d:%02d:%02d",
        _date.Month,
        _date.Date,
        _date.Year,
        _time.Hours,
        _time.Minutes,
        _time.Seconds
    );
    serialDevice->println(stimeStr);
#endif

    return true;
}

uint32_t i2cCallback(cmd *c) {
    // scan for connected i2c modules
    // derived from https://learn.adafruit.com/scanning-i2c-addresses/arduino
    Wire.begin(bruceConfigPins.i2c_bus.sda, bruceConfigPins.i2c_bus.scl);
    byte error, address;
    int nDevices;
    serialDevice->println("Scanning...");
    nDevices = 0;
    for (address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            serialDevice->print("I2C device found at address 0x");
            if (address < 16) serialDevice->print("0");
            serialDevice->print(address, HEX);
            nDevices++;
        } else if (error == 4) {
            serialDevice->print("Unknown error at address 0x");
            if (address < 16) serialDevice->print("0");
            serialDevice->println(address, HEX);
        }
    }

    if (nDevices == 0) {
        serialDevice->println("No I2C devices found");
        return false;
    }

    return true;
}

uint32_t freeCallback(cmd *c) {
    serialDevice->print("Total heap: ");
    serialDevice->println(ESP.getHeapSize());
    serialDevice->print("Free heap: ");
    serialDevice->println(ESP.getFreeHeap());

    if (psramFound()) {
        serialDevice->print("Total PSRAM: ");
        serialDevice->println(ESP.getPsramSize());
        serialDevice->print("Free PSRAM: ");
        serialDevice->println(ESP.getFreePsram());
    }

    return true;
}

uint32_t infoCallback(cmd *c) {
    serialDevice->print("Bruce v");
    serialDevice->println(BRUCE_VERSION);
    serialDevice->println(GIT_COMMIT_HASH);
    serialDevice->print("SDK: ");              
    serialDevice->println(ESP.getSdkVersion());
    serialDevice->println("MAC addr: " + String(WiFi.macAddress()));
    // https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/ChipID/GetChipID/GetChipID.ino
    // serialDevice->printf("Chip is %s (revision v%d)\n", ESP.getChipModel(), ESP.getChipRevision());
    // serialDevice->printf("Detected flash size: %d\n", ESP.getFlashChipSize());
    // serialDevice->printf("This chip has %d cores\n", ESP.getChipCores());
    // serialDevice->printf("CPU Freq is %d\n", ESP.getCpuFreqMHz());
    // Features: WiFi, BLE, Embedded Flash 8MB (GD)
    // Crystal is 40MHz
    // MAC: 24:58:7c:5b:24:5c

    if (wifiConnected) {
        serialDevice->println("Wifi: connected");
        serialDevice->println("Ip: " + wifiIP); // read global var
    } else {
        serialDevice->println("Wifi: not connected");
    }

    serialDevice->println("Device: " + String(DEVICE_NAME));

    return true;
}

uint32_t helpCallback(cmd *c) {
    serialDevice->print("Bruce v");
    serialDevice->print(BRUCE_VERSION);
    serialDevice->print("\nThese shell commands are defined internally.\n");

    serialDevice->println("\nWiFi Commands:");
    serialDevice->println("  wifi off (Disconnects Wifi)");
    serialDevice->println(
        "  wifi on  (Connects to a known Wifi network. if there's no known network, starts in AP Mode)"
    );
    serialDevice->println("  wifi add \"SSID\" \"Password\" (adds a network to the list)");
    serialDevice->println("  arp - Starts Scan Hosts ARP Scanner");
    serialDevice->println("  listen   - Starts listening TCP default port");
    serialDevice->println("  sniffer - Starts Raw Sniffer");
    serialDevice->println("\nWebUI Commands:");
    serialDevice->println("  webui      - WebUI Webserver start");
    serialDevice->println("\nIR Commands:");
    serialDevice->println("  ir rx <timeout>      - Read an IR signal and print the dump on serialDevice->");
    serialDevice->println(
        "  ir rx raw <timeout>  - Read an IR signal in RAW mode and print the dump on serialDevice->"
    );
    serialDevice->println("  ir tx <protocol> <address> <decoded_value>  - Send a custom decoded IR signal.");
    serialDevice->println(
        "  ir tx_from_file <ir file path> [hide default UI true/false] - Send an IR signal saved in "
        "storage. Optionally hide the default UI."
    );

    serialDevice->println("\nRF Commands:");
    serialDevice->println(
        "  subghz rx <timeout>       - Read an RF signal and print the dump on serialDevice-> (alias: rf rx)"
    );
    serialDevice->println(
        "  subghz rx raw <timeout>   - Read an RF signal in RAW mode and print the dump on serialDevice-> "
        "(alias: "
        "rf rx raw)"
    );
    serialDevice->println(
        "  subghz tx <decoded_value> <frequency> <te> <count>  - Send a custom decoded RF signal. (alias: rf "
        "tx)"
    );
    serialDevice->println(
        "  subghz tx_from_file <sub file path> [hide default UI true/false] - Send an RF signal "
        "saved in storage. Optionally hide the default UI."
    );

    serialDevice->println("\nAudio Commands:");
    serialDevice->println("  music_player <audio file path>  - Play an audio file.");
    serialDevice->println("  tone <frequency> <duration>  - Play a single squarewave audio tone.");
    serialDevice->println("  say <text>   - Text-To-Speech (speaker required).");

    serialDevice->println("\nUI Commands:");
    serialDevice->println("  led <r/g/b> <0-255>    - Change the UI main color.");
    serialDevice->println("  clock                 - Show the clock UI.");

    serialDevice->println("\nPower Management:");
    serialDevice->println("  power <off/reboot/sleep>  - General power management.");

    serialDevice->println("\nGPIO Commands:");
    serialDevice->println("  gpio mode <pin number> <0/1>  - Set GPIO pins mode (0=input, 1=output).");
    serialDevice->println("  gpio set <pin number> <0/1>   - Direct GPIO pins control (0=off, 1=on).");

    serialDevice->println("\nI2C and Storage:");
    serialDevice->println("  i2c scan                - Scan for modules connected to the I2C bus.");
    serialDevice->println(
        "  storage <list/remove/mkdir/rename/read/write/copy/md5/crc32> <file path>  - Common file "
        "management commands."
    );
    serialDevice->println("  ls - Same as storage list");

    serialDevice->println("\nSettings:");
    serialDevice->println("  settings                - View all the current settings.");
    serialDevice->println("  settings <name>         - View a single setting value.");
    serialDevice->println("  settings <name> <new value>  - Alter a single setting value.");
    serialDevice->println("  factory_reset           - Reset to default configuration.");

    return true;
}

void optionsList() {
    int i = 0;
    Serial.println("\nActual Menu: " + menuOptionLabel);
    Serial.println("Options available: ");
    for (auto opt : options) {
        String txt = (opt.hovered ? ">" : " ") + String(i) + " - " + opt.label;
        Serial.println(txt);
        i++;
    }
}

uint32_t navCallback(cmd *c) {
    Command cmd(c);
    volatile bool *var = &NextPress;
    Argument arg = cmd.getArgument("command");
    Argument arg2 = cmd.getArgument("duration");
    String _dur = arg2.getValue();
    _dur.trim();
    int dur = 1;

    if (_dur.length() > 0) {
        dur = arg2.getValue().toInt();
        if (dur < 0) dur = 1; // heavy fingers on the remote interface
    }
    String nav = arg.getValue();
    nav.trim();

    // Here send press response only to USB serial to avoid problems with BLE app
    if (nav == "next") {
        Serial.println("Next Pressed");
        var = &NextPress;
    } else if (nav == "prev") {
        Serial.println("Prev Pressed");
        var = &PrevPress;
    } else if (nav == "esc") {
        Serial.println("Esc Pressed");
        var = &EscPress;
    } else if (nav == "up") {
        Serial.println("Up Pressed");
        var = &UpPress;
    } else if (nav == "down") {
        Serial.println("Down Pressed");
        var = &DownPress;
    } else if (nav == "select" || nav == "sel") {
        Serial.println("Select Pressed");
        var = &SelPress;
    } else if (nav == "nextpage") {
        Serial.println("Next Page Pressed");
        var = &NextPagePress;
    } else if (nav == "prevpage") {
        Serial.println("Prev Page Pressed");
        var = &PrevPagePress;
    } else {
        serialDevice->println(
            "Unknown command, use: \n\"nav Next\" or \n\"nav Prev\" or \n\"nav Esc\" or \n\"nav Select\" or "
            "\n\"nav Up\" or \n\"nav Down\" or \n\"nav NextPage\" or \n\"nav PrevPage\""
        );
        return false;
    }
    // wakeUpScreen(); // Do not wakeup screen if it is dimmed and using Remote control
    unsigned long tmp = millis();
    while (millis() <= tmp + dur) {
        if (*var == false) {
            AnyKeyPress = true;
            SerialCmdPress = true;
            *var = true;
            if (!LongPress) vTaskDelay(190 / portTICK_PERIOD_MS);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    tmp = millis() - tmp;
    Serial.printf("and Released after %lums", tmp);
    optionsList();

    return true;
}

uint32_t optionsCallback(cmd *c) {
    Command cmd(c);
    Argument arg = cmd.getArgument("run");
    // int opt = arg.getValue().startsWith("-") ? -1 : arg.getValue().toInt();
    int opt = arg.getValue().toInt();

    if (opt >= 0 && opt < options.size()) {
        // wakeUpScreen(); // Do not wakeup screen if it is dimmed and using Remote control
        forceMenuOption = opt;
        serialDevice->printf(
            "Selected option %d: %s\n", forceMenuOption, options[forceMenuOption].label.c_str()
        );
        vTaskDelay(30 / portTICK_PERIOD_MS);
        optionsList();
    } else if (options.size() > 0) {
        optionsList();
    } else serialDevice->println("No options Available");
    return true;
}
uint32_t optionsJsonCallback(cmd *c) {
    String response = getOptionsJSON(); // core/utils.h
    serialDevice->println(response);
    return true;
}

uint32_t displayCallback(cmd *c) {
    Command cmd(c);
    Argument arg = cmd.getArgument("option");
    String opt = arg.getValue();
    if (opt == "start") {
        serialDevice->println("Display: Started async serial");
        tft.startAsyncSerial();
        tft.getTftInfo();
    } else if (opt == "stop") {
        serialDevice->println("Display: Stopped async serial");
        tft.stopAsyncSerial();
    } else if (opt == "status") {
        if (tft.getLogging()) serialDevice->println("Display: Logging tft is ACTIVATED");
        else serialDevice->println("Display: Logging tft is DEACTIVATED");
    } else if (opt == "dump") {
        uint8_t binData[MAX_LOG_ENTRIES * MAX_LOG_SIZE];
        size_t binSize = 0;
        tft.getBinLog(binData, binSize);

        serialDevice->println("Binary Dump:");
        for (size_t i = 0; i < binSize; i++) {
            if (i % 16 == 0) serialDevice->println();
            // if (i % 16 == 0) serialDevice->printf("\n%04X: ", i);
            serialDevice->printf("%02X ", binData[i]);
        }
        serialDevice->println("\n[End of Dump]");
    } else if (opt == "info") {
        serialDevice->println(TFT_WIDTH + String("x") + TFT_HEIGHT + String("x") + ROTATION);
    } else {
        serialDevice->println(
            "Display command accept:\n"
            "display start : Start Logging\n"
            "display stop  : Stop Logging\n"
            "display status: Get Logging state\n"
            "display dump  : Dumps binary log"
            "display info  : Get display info"
        );
        return false;
    }
    return true;
}

uint32_t loaderCallback(cmd *c) {
    Command cmd(c);
    String arg = cmd.getArgument("cmd").getValue();
    String appname = cmd.getArgument("appname").getValue();

    std::vector<MenuItemInterface *> _menuItems = mainMenu.getItems();
    int _totalItems = _menuItems.size();

    if (arg == "list") {
        for (int i = 0; i < _totalItems; i++) { serialDevice->println(_menuItems[i]->getName()); }
        serialDevice->println("BadUSB");
        serialDevice->println("WebUI");
        serialDevice->println("LittleFS");
        return true;

    } else if (arg == "open") {
        if (!appname.isEmpty()) {
            // look for a matching name
            for (int i = 0; i < _totalItems; i++) {
                if (appname.equalsIgnoreCase(_menuItems[i]->getName())) {
                    // open the associated app
                    _menuItems[i]->optionsMenu();
                    return true;
                }
            }
// additional shortcuts
#if !defined(LITE_VERSION)
            if (appname.equalsIgnoreCase("badusb")) {
                ducky_setup(hid_usb, false);
                return true;
            } else
#endif
                if (appname.equalsIgnoreCase("webui")) {
                loopOptionsWebUi();
                return true;
            } else if (appname.equalsIgnoreCase("littlefs")) {
                loopSD(LittleFS);
                return true;
            }
            // else no matching app name found
            serialDevice->println("app not found: " + appname);
            return false;
        }

    } else {
        serialDevice->println(
            "Loader command accept:\n"
            "loader list : Lists available applications\n"
            "loader open appname  : Runs the entered application.\n"
        );
        return false;
    }

    // TODO: close: Closes the running application.
    // TODO: info: Displays the loader’s state.
    return false;
}

void createUtilCommands(SimpleCLI *cli) {
    cli->addCommand("uptime", uptimeCallback);
    cli->addCommand("date", dateCallback);
    cli->addCommand("i2c", i2cCallback);
    cli->addCommand("free", freeCallback);
    cli->addCommand("info,!,device_info", infoCallback);
    cli->addCommand("help,?,halp", helpCallback);
    cli->addCommand("optionsJSON", optionsJsonCallback);
    Command display = cli->addCommand("display", displayCallback);
    display.addPosArg("option", "dump");

    Command navigation = cli->addCommand("nav,navigate,navigation", navCallback);
    navigation.addPosArg("command");
    navigation.addPosArg("duration", "1");

    Command opt = cli->addCommand("options,option", optionsCallback);
    opt.addPosArg("run", "-1");

    Command loader = cli->addCommand("loader", loaderCallback);
    loader.addPosArg("cmd");
    loader.addPosArg("appname", "none"); // optional
}
