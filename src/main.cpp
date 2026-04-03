#include "core/main_menu.h"
#include <globals.h>

#include "core/powerSave.h"
#include "core/serial_commands/cli.h"
#include "core/utils.h"
#include "current_year.h"
#include "esp32-hal-psram.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include <functional>
#include <string>
#include <vector>
io_expander ioExpander;
BruceConfig bruceConfig;
BruceConfigPins bruceConfigPins;

SerialCli serialCli;
USBSerial USBserial;
SerialDevice *serialDevice = &USBserial;

StartupApp startupApp;
String startupAppJSInterpreterFile = "";

MainMenu mainMenu;
SPIClass sdcardSPI;
#ifdef USE_HSPI_PORT
#ifndef VSPI
#define VSPI FSPI
#endif
SPIClass CC_NRF_SPI(VSPI);
#else
SPIClass CC_NRF_SPI(HSPI);
#endif

// Navigation Variables
volatile bool NextPress = false;
volatile bool PrevPress = false;
volatile bool UpPress = false;
volatile bool DownPress = false;
volatile bool SelPress = false;
volatile bool EscPress = false;
volatile bool AnyKeyPress = false;
volatile bool NextPagePress = false;
volatile bool PrevPagePress = false;
volatile bool LongPress = false;
volatile bool SerialCmdPress = false;
volatile int forceMenuOption = -1;
volatile uint8_t menuOptionType = 0;
String menuOptionLabel = "";
#ifdef HAS_ENCODER_LED
volatile int EncoderLedChange = 0;
#endif

TouchPoint touchPoint;

keyStroke KeyStroke;

TaskHandle_t xHandle;
void __attribute__((weak)) taskInputHandler(void *parameter) {
    auto timer = millis();
    while (true) {
        checkPowerSaveTime();
        // Sometimes this task run 2 or more times before looptask,
        // and navigation gets stuck, the idea here is run the input detection
        // if AnyKeyPress is false, or rerun if it was not renewed within 75ms (arbitrary)
        // because AnyKeyPress will be true if didn´t passed through a check(bool var)
        if (!AnyKeyPress || millis() - timer > 75) {
            NextPress = false;
            PrevPress = false;
            UpPress = false;
            DownPress = false;
            SelPress = false;
            EscPress = false;
            AnyKeyPress = false;
            SerialCmdPress = false;
            NextPagePress = false;
            PrevPagePress = false;
            touchPoint.pressed = false;
            touchPoint.Clear();
#ifndef USE_TFT_eSPI_TOUCH
            InputHandler();
#endif
            timer = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// Public Globals Variables
unsigned long previousMillis = millis();
int prog_handler; // 0 - Flash, 1 - LittleFS, 3 - Download
String cachedPassword = "";
int8_t interpreter_state = -1;
bool sdcardMounted = false;
bool gpsConnected = false;

// wifi globals
// TODO put in a namespace
bool wifiConnected = false;
bool isWebUIActive = false;
String wifiIP;

bool BLEConnected = false;
bool returnToMenu;
bool isSleeping = false;
bool isScreenOff = false;
bool isScreensaverActive = false;
bool dimmer = false;
char timeStr[12];
time_t localTime;
struct tm *timeInfo;
#if defined(HAS_RTC)
#if defined(HAS_RTC_PCF85063A)
pcf85063_RTC _rtc;
#else
cplus_RTC _rtc;
#endif
RTC_TimeTypeDef _time;
RTC_DateTypeDef _date;
bool clock_set = true;
#else
ESP32Time rtc;
bool clock_set = false;
#endif

std::vector<Option> options;
// Protected global variables
#if defined(HAS_SCREEN)
tft_logger tft = tft_logger(); // Invoke custom library
tft_sprite sprite = tft_sprite(&tft);
tft_sprite draw = tft_sprite(&tft);
volatile int tftWidth = TFT_HEIGHT;
#ifdef HAS_TOUCH
volatile int tftHeight =
    TFT_WIDTH - 20; // 20px to draw the TouchFooter(), were the btns are being read in touch devices.
#else
volatile int tftHeight = TFT_WIDTH;
#endif
#else
tft_logger tft;
SerialDisplayClass &sprite = tft;
SerialDisplayClass &draw = tft;
volatile int tftWidth = VECTOR_DISPLAY_DEFAULT_HEIGHT;
volatile int tftHeight = VECTOR_DISPLAY_DEFAULT_WIDTH;
#endif

#include "core/display.h"
#include "core/led_control.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/serialcmds.h"
#include "core/settings.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include "modules/bjs_interpreter/interpreter.h" // for JavaScript interpreter
#include "modules/others/audio.h"                // for playAudioFile
#include "modules/rf/rf_utils.h"                 // for initCC1101once
#include <Wire.h>

/*********************************************************************
 **  Function: begin_storage
 **  Config LittleFS and SD storage
 *********************************************************************/
void begin_storage() {
    if (!LittleFS.begin(true)) { LittleFS.format(), LittleFS.begin(); }
    bool checkFS = setupSdCard();
    bruceConfig.fromFile(checkFS);
    bruceConfigPins.fromFile(checkFS);
}

/*********************************************************************
 **  Function: _setup_gpio()
 **  Sets up a weak (empty) function to be replaced by /ports/* /interface.h
 *********************************************************************/
void _setup_gpio() __attribute__((weak));
void _setup_gpio() {}

/*********************************************************************
 **  Function: _post_setup_gpio()
 **  Sets up a weak (empty) function to be replaced by /ports/* /interface.h
 *********************************************************************/
void _post_setup_gpio() __attribute__((weak));
void _post_setup_gpio() {}

/*********************************************************************
 **  Function: setup_gpio
 **  Setup GPIO pins
 *********************************************************************/
void setup_gpio() {

    // init setup from /ports/*/interface.h
    _setup_gpio();

    // Smoochiee v2 uses a AW9325 tro control GPS, MIC, Vibro and CC1101 RX/TX powerlines
    ioExpander.init(IO_EXPANDER_ADDRESS, &Wire);

#if TFT_MOSI > 0
    if (bruceConfigPins.CC1101_bus.mosi == (gpio_num_t)TFT_MOSI)
        initCC1101once(&tft.getSPIinstance()); // (T_EMBED), CORE2 and others
    else
#endif
        if (bruceConfigPins.CC1101_bus.mosi == bruceConfigPins.SDCARD_bus.mosi)
        initCC1101once(&sdcardSPI); // (ARDUINO_M5STACK_CARDPUTER) and (ESP32S3DEVKITC1) and devices that
                                    // share CC1101 pin with only SDCard
    else initCC1101once(NULL);
    // (ARDUINO_M5STICK_C_PLUS) || (ARDUINO_M5STICK_C_PLUS2) and others that doesn´t share SPI with
    // other devices (need to change it when Bruce board comes to shore)
}

/*********************************************************************
 **  Function: begin_tft
 **  Config tft
 *********************************************************************/
void begin_tft() {
    tft.setRotation(bruceConfigPins.rotation); // sometimes it misses the first command
    tft.invertDisplay(bruceConfig.colorInverted);
    tft.setRotation(bruceConfigPins.rotation);
    tftWidth = tft.width();
#ifdef HAS_TOUCH
    tftHeight = tft.height() - 20;
#else
    tftHeight = tft.height();
#endif
    resetTftDisplay();
    // NOTE: brightness is intentionally NOT set here.
    // boot_screen_anim() fades in the backlight as part of the animation.
    // If instantBoot is true, the animation sets it before returning.
    currentScreenBrightness = 0;
}

/*********************************************************************
 **  Function: boot_screen
 **  Draw boot screen
 *********************************************************************/
/*********************************************************************
 **  Helper: lerpColor565
 **  Linearly interpolate between two RGB565 colours. t = 0–255.
 *********************************************************************/
static uint16_t lerpColor565(uint16_t a, uint16_t b, uint8_t t) {
    uint8_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    uint8_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    uint8_t r  = ar + ((int)(br - ar) * t / 255);
    uint8_t g  = ag + ((int)(bg - ag) * t / 255);
    uint8_t bv = ab + ((int)(bb - ab) * t / 255);
    return (r << 11) | (g << 5) | bv;
}

/*********************************************************************
 **  Helper: _boot_draw_splash
 **  Draw the static splash frame entirely while backlight is OFF.
 **  No flickering possible — screen is rendered blind.
 *********************************************************************/
static void _boot_draw_splash() {
    uint16_t pri = bruceConfig.priColor;
    uint16_t bg  = bruceConfig.bgColor;
    uint16_t dim = lerpColor565(bg, pri, 80); // subtle accent

    tft.fillScreen(bg);

    // ── Corner accent brackets ────────────────────────────────────────
    const int M = 8;  // margin
    const int L = 18; // bracket arm length
    // top-left
    tft.drawFastHLine(M, M, L, pri);
    tft.drawFastVLine(M, M, L, pri);
    // top-right
    tft.drawFastHLine(tftWidth - M - L, M, L, pri);
    tft.drawFastVLine(tftWidth - M - 1, M, L, pri);
    // bottom-left
    tft.drawFastHLine(M, tftHeight - M - 1, L, pri);
    tft.drawFastVLine(M, tftHeight - M - L, L, pri);
    // bottom-right
    tft.drawFastHLine(tftWidth - M - L, tftHeight - M - 1, L, pri);
    tft.drawFastVLine(tftWidth - M - 1, tftHeight - M - L, L, pri);

    // ── "BRUCE" centred, big ──────────────────────────────────────────
    tft.setTextSize(FM);
    tft.setTextColor(pri, bg);
    int titleY = tftHeight / 2 - FM * 6 - 8;
    tft.drawCentreString("BRUCE", tftWidth / 2, titleY, 1);

    // ── Accent line under title ────────────────────────────────────────
    int lineY = titleY + FM * 14 + 4;
    tft.drawFastHLine(M + L + 4, lineY, tftWidth - 2 * (M + L + 4), pri);
    // thinner ghost line 2px below for depth
    tft.drawFastHLine(M + L + 4 + 6, lineY + 2, tftWidth - 2 * (M + L + 4) - 12, dim);

    // ── Version + subtitle ─────────────────────────────────────────────
    tft.setTextSize(FP);
    tft.setTextColor(dim, bg);
    tft.drawCentreString("PREDATORY FIRMWARE", tftWidth / 2, lineY + 6, 1);
    tft.setTextColor(lerpColor565(bg, pri, 120), bg);
    tft.drawCentreString(BRUCE_VERSION, tftWidth / 2, lineY + 6 + FP * 9 + 2, 1);
}

/*********************************************************************
 **  Function: boot_screen_anim
 **
 **  Flow (no custom image):
 **    • Draw splash with backlight OFF  → zero flicker possible
 **    • Fade backlight 0 → bright       (~700 ms smooth reveal)
 **    • Accent line pulse               (~1200 ms)
 **    • Scanline wipe down to clear     (~300 ms)
 **
 **  Custom boot.jpg / boot.gif: behaviour unchanged.
 **  instantBoot: skips animation, sets brightness directly.
 *********************************************************************/
void boot_screen_anim() {
    // ── Handle instantBoot ─────────────────────────────────────────────
    if (bruceConfig.instantBoot) {
        tft.fillScreen(bruceConfig.bgColor);
        _setBrightness(bruceConfig.bright);
        currentScreenBrightness = bruceConfig.bright;
        return;
    }

    // ── Custom boot image path (unchanged behaviour) ───────────────────
    int boot_img = 0;
    if (sdcardMounted) {
        if      (SD.exists("/boot.jpg")) boot_img = 1;
        else if (SD.exists("/boot.gif")) boot_img = 3;
    }
    if (boot_img == 0 && LittleFS.exists("/boot.jpg")) boot_img = 2;
    if (boot_img == 0 && LittleFS.exists("/boot.gif")) boot_img = 4;
    if (bruceConfig.theme.boot_img) boot_img = 5;

    if (boot_img > 0) {
        // Éteindre avant de dessiner l'image de boot
        _setBrightness(0);
        currentScreenBrightness = 0;
        tft.fillScreen(bruceConfig.bgColor);
        if      (boot_img == 5) drawImg(*bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.boot_img), 0, 0, true, 3600);
        else if (boot_img == 1) drawImg(SD, "/boot.jpg", 0, 0, true);
        else if (boot_img == 2) drawImg(LittleFS, "/boot.jpg", 0, 0, true);
        else if (boot_img == 3) drawImg(SD, "/boot.gif", 0, 0, true, 3600);
        else if (boot_img == 4) drawImg(LittleFS, "/boot.gif", 0, 0, true, 3600);
        // Fade-in de l'image de boot (ultra-fluide)
        for (int b = 0; b <= bruceConfig.bright; b++) {
            _setBrightness(b);
            delay(3);
        }
        currentScreenBrightness = bruceConfig.bright;
        unsigned long t0 = millis();
        while (millis() - t0 < 4000) {
            if (check(AnyKeyPress)) break;
            delay(20);
        }
        // Fade-out de l'image vers le menu (ultra-fluide)
        for (int b = bruceConfig.bright; b >= 0; b--) {
            _setBrightness(b);
            delay(3);
        }
        currentScreenBrightness = 0;
        return;
    }

#if defined(LITE_VERSION)
    tft.fillScreen(bruceConfig.bgColor);
    _setBrightness(bruceConfig.bright);
    currentScreenBrightness = bruceConfig.bright;
    return;
#else

    uint16_t pri = bruceConfig.priColor;
    uint16_t bg  = bruceConfig.bgColor;

    // ── 1. Draw splash while backlight is OFF ─────────────────────────
    // This runs blind — no flicker at all.
    _boot_draw_splash();

    // ── 2. Fade backlight in (0 → bruceConfig.bright, 12ms delay like screensaver) ──────
    int target = bruceConfig.bright;
    bool skipped = false;
    for (int b = 0; b <= target && !skipped; b++) {
        _setBrightness(b);
        delay(12);
        if (check(AnyKeyPress)) { _setBrightness(target); skipped = true; }// Use _setBrightness directly
    }
    currentScreenBrightness = target;

    // ── 3. Accent line pulse (3 slow beats, ~1200 ms) ──────────────
    // Fixed: use secColor for visible pulsation effect
    if (!skipped) {
        int lineY  = (tftHeight / 2 - FM * 6 - 8) + FM * 14 + 4;
        const int M = 8, L = 18;
        int lineX1 = M + L + 4, lineW = tftWidth - 2 * (M + L + 4);

        // Use secColor as pulse target for better visual contrast
        uint16_t pulseTarget = bruceConfig.secColor;

        for (int beat = 0; beat < 3 && !skipped; beat++) {
            for (int s = 0; s <= 255 && !skipped; s += 8) {
                tft.drawFastHLine(lineX1, lineY, lineW,
                    lerpColor565(pri, pulseTarget, (uint8_t)s));
                delay(5);
                if (check(AnyKeyPress)) skipped = true;
            }
            for (int s = 255; s >= 0 && !skipped; s -= 8) {
                tft.drawFastHLine(lineX1, lineY, lineW,
                    lerpColor565(pri, pulseTarget, (uint8_t)s));
                delay(5);
                if (check(AnyKeyPress)) skipped = true;
            }
        }
    }

    // ── 4. Scanline wipe-out downward (~300 ms) ──────────────────────
    // Fixed: proper scanline wipe without visual artifacts
    for (int y = 0; y <= tftHeight; y += 2) {
        tft.drawFastHLine(0, y,     tftWidth, pri);
        tft.drawFastHLine(0, y + 1, tftWidth, pri);
        // Fill cleared area above the current scanline position
        if (y > 2) tft.fillRect(0, 0, tftWidth, y - 1, bg);
        delay(3);
    }

    tft.fillScreen(bg);
#endif // LITE_VERSION
}

/*********************************************************************
 **  Function: menu_entry_anim
 **
 **  Animation fade-out/fade-in smooth (12ms) lors de l'affichage du menu principal
 **  S'exécute uniquement au premier démarrage (cold boot depuis deep sleep)
 *********************************************************************/
void menu_entry_anim() {
#if !defined(LITE_VERSION)
    static bool firstEntry = true;
    bool isColdBoot = firstEntry;
    firstEntry = false;

    // Sauter uniquement si ce n'est pas un cold boot (retour de sous-menu, etc.)
    if (!isColdBoot) return;

    int savedBright = (currentScreenBrightness > 0) ? currentScreenBrightness : bruceConfig.bright;

    // Fade-out smooth 12ms
    for (int b = savedBright; b >= 0; b--) {
        _setBrightness(b);
        delay(12);
    }
    // Fade-in smooth 12ms
    for (int b = 0; b <= savedBright; b++) {
        _setBrightness(b);
        delay(12);
    }
    currentScreenBrightness = savedBright;
#endif
}

/*********************************************************************
 **  Function: init_clock
 **  Clock initialisation for propper display in menu
 *********************************************************************/
void init_clock() {
#if defined(HAS_RTC)
    _rtc.begin();
#if defined(HAS_RTC_BM8563)
    _rtc.GetBm8563Time();
#endif
#if defined(HAS_RTC_PCF85063A)
    _rtc.GetPcf85063Time();
#endif
    _rtc.GetTime(&_time);
    _rtc.GetDate(&_date);

    struct tm timeinfo = {};
    timeinfo.tm_sec = _time.Seconds;
    timeinfo.tm_min = _time.Minutes;
    timeinfo.tm_hour = _time.Hours;
    timeinfo.tm_mday = _date.Date;
    timeinfo.tm_mon = _date.Month > 0 ? _date.Month - 1 : 0;
    timeinfo.tm_year = _date.Year >= 1900 ? _date.Year - 1900 : 0;
    time_t epoch = mktime(&timeinfo);
    struct timeval tv = {.tv_sec = epoch};
    settimeofday(&tv, nullptr);
#else
    struct tm timeinfo = {};
    timeinfo.tm_year = CURRENT_YEAR - 1900;
    timeinfo.tm_mon = 0x05;
    timeinfo.tm_mday = 0x14;
    time_t epoch = mktime(&timeinfo);
    rtc.setTime(epoch);
    clock_set = true;
    struct timeval tv = {.tv_sec = epoch};
    settimeofday(&tv, nullptr);
#endif
}

/*********************************************************************
 **  Function: init_led
 **  Led initialisation
 *********************************************************************/
void init_led() {
#ifdef HAS_RGB_LED
    beginLed();
#endif
}

/*********************************************************************
 **  Function: startup_sound
 **  Play sound or tone depending on device hardware
 *********************************************************************/
void startup_sound() {
    if (bruceConfig.soundEnabled == 0) return; // if sound is disabled, do not play sound
#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN)
    // Bip M5 just because it can. Does not bip if splashscreen is bypassed
    _tone(5000, 50);
    delay(200);
    _tone(5000, 50);
    /*  2fix: menu infinite loop */
#elif defined(HAS_NS4168_SPKR)
    // play a boot sound
    if (bruceConfig.theme.boot_sound) {
        playAudioFile(bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.boot_sound));
    } else if (SD.exists("/boot.wav")) {
        playAudioFile(&SD, "/boot.wav");
    } else if (LittleFS.exists("/boot.wav")) {
        playAudioFile(&LittleFS, "/boot.wav");
    }
#endif
#endif
}

/*********************************************************************
 **  Function: setup
 **  Where the devices are started and variables set
 *********************************************************************/
void setup() {
    Serial.setRxBufferSize(
        SAFE_STACK_BUFFER_SIZE / 4
    ); // Must be invoked before Serial.begin(). Default is 256 chars
    Serial.begin(115200);

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    if (psramInit()) log_d("PSRAM Started");
    if (psramFound()) log_d("PSRAM Found");
    else log_d("PSRAM Not Found");
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());

    // declare variables
    prog_handler = 0;
    sdcardMounted = false;
    wifiConnected = false;
    BLEConnected = false;
    bruceConfig.bright = 100; // theres is no value yet
    bruceConfigPins.rotation = ROTATION;
    setup_gpio();
#if defined(HAS_SCREEN)
    // Kill backlight BEFORE tft.init() — prevents random VRAM garbage from
    // flashing on screen while the display controller initialises.
#if defined(TFT_BL) && TFT_BL >= 0
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
#endif
    tft.init();
    tft.setRotation(bruceConfigPins.rotation);
    tft.fillScreen(TFT_BLACK);
    // Backlight is off — screen is dark while storage/config loads
#else
    tft.begin();
#endif
    begin_storage();
    begin_tft();
    init_clock();
    init_led();

    options.reserve(20); // preallocate some options space to avoid fragmentation

    // Set WiFi country to avoid warnings and ensure max power
    const wifi_country_t country = {
        .cc = "US",
        .schan = 1,
        .nchan = 14,
#ifdef CONFIG_ESP_PHY_MAX_TX_POWER
        .max_tx_power = CONFIG_ESP_PHY_MAX_TX_POWER, // 20
#endif
        .policy = WIFI_COUNTRY_POLICY_MANUAL
    };

    esp_wifi_set_max_tx_power(80); // 80 translates to 20dBm
    esp_wifi_set_country(&country);

    // Some GPIO Settings (such as CYD's brightness control must be set after tft and sdcard)
    _post_setup_gpio();
    // end of post gpio begin

    // #ifndef USE_TFT_eSPI_TOUCH
    // This task keeps running all the time, will never stop
    xTaskCreate(
        taskInputHandler,              // Task function
        "InputHandler",                // Task Name
        INPUT_HANDLER_TASK_STACK_SIZE, // Stack size
        NULL,                          // Task parameters
        2,                             // Task priority (0 to 3), loopTask has priority 2.
        &xHandle                       // Task handle (not used)
    );
    // #endif
#if defined(HAS_SCREEN)
    bruceConfig.openThemeFile(bruceConfig.themeFS(), bruceConfig.themePath, false);
    if (!bruceConfig.instantBoot) {
        boot_screen_anim();
        startup_sound();
    }
    if (bruceConfig.wifiAtStartup) {
        log_i("Loading Wifi at Startup");
        xTaskCreate(
            wifiConnectTask,   // Task function
            "wifiConnectTask", // Task Name
            4096,              // Stack size
            NULL,              // Task parameters
            2,                 // Task priority (0 to 3), loopTask has priority 2.
            NULL               // Task handle (not used)
        );
    }
#endif
    //  start a task to handle serial commands while the webui is running
    startSerialCommandsHandlerTask(true);

    wakeUpScreen();
    if (bruceConfig.startupApp != "" && !startupApp.startApp(bruceConfig.startupApp)) {
        bruceConfig.setStartupApp("");
    }
}

/**********************************************************************
 **  Function: loop
 **  Main loop
 **********************************************************************/
#if defined(HAS_SCREEN)
void loop() {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    if (interpreter_state > 0) {
        vTaskDelete(serialcmdsTaskHandle); // stop serial commands while in interpreter
        vTaskDelay(pdMS_TO_TICKS(10));
        interpreter_state = 2;
        Serial.println("Entering interpreter...");
        while (interpreter_state > 0) { vTaskDelay(pdMS_TO_TICKS(500)); }
        Serial.println("Exiting interpreter...");
        if (interpreter_state == -1) { interpreterTaskHandler = NULL; }
        startSerialCommandsHandlerTask();
        previousMillis = millis(); // ensure that will not dim screen when get back to menu
    }
#endif

    // Animation fade-in maintenant dans MainMenu::begin()
    tft.fillScreen(bruceConfig.bgColor);
    mainMenu.begin();
    delay(1);
}
#else

void loop() {
    tft.setLogging();
    Serial.println(
        "\n"
        "██████  ██████  ██    ██  ██████ ███████ \n"
        "██   ██ ██   ██ ██    ██ ██      ██      \n"
        "██████  ██████  ██    ██ ██      █████   \n"
        "██   ██ ██   ██ ██    ██ ██      ██      \n"
        "██████  ██   ██  ██████   ██████ ███████ \n"
        "                                         \n"
        "         PREDATORY FIRMWARE\n\n"
        "Tips: Connect to the WebUI for better experience\n"
        "      Add your network by sending: wifi add ssid password\n\n"
        "At your command:"
    );

    // Enable navigation through webUI
    tft.fillScreen(bruceConfig.bgColor);
    mainMenu.begin();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
#endif
