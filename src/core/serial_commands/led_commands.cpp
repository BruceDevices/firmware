#include "led_commands.h"
#include <globals.h>

#ifdef HAS_RGB_LED
#include "core/led_control.h"

// The effect task reads the color/effect straight from bruceConfig (led_control.cpp),
// so these commands go through the config setters and then re-apply, exactly like the
// LED settings menu does. That also makes the change survive a reboot.
static void applyLedColor(uint32_t color) {
    bruceConfig.setLedColor(color);
    ledSetup();
    serialDevice->printf("LED color set to %06X\n", (unsigned int)(color & 0xFFFFFF));
}

static bool parseByteArg(Command &cmd, const char *name, int &out) {
    Argument arg = cmd.getArgument(name);
    String strValue = arg.getValue();
    strValue.trim();

    out = atoi(strValue.c_str());
    if (out < 0 || out > 255) {
        serialDevice->println("Invalid value: " + strValue + " (expected 0-255)");
        return false;
    }
    return true;
}

// Keeps the other two channels untouched, so "led r 255" then "led g 255" gives yellow.
static uint32_t setLedChannel(cmd *c, uint8_t shift) {
    Command cmd(c);

    int value = 0;
    if (!parseByteArg(cmd, "value", value)) return false;

    uint32_t color = bruceConfig.ledColor & ~((uint32_t)0xFF << shift);
    applyLedColor(color | ((uint32_t)value << shift));
    return true;
}

// e.g. "led r 255", "led g 128", "led b 0"
static uint32_t ledRedCallback(cmd *c) { return setLedChannel(c, 16); }
static uint32_t ledGreenCallback(cmd *c) { return setLedChannel(c, 8); }
static uint32_t ledBlueCallback(cmd *c) { return setLedChannel(c, 0); }

// e.g. "led rgb 255 0 255"
static uint32_t ledRgbCallback(cmd *c) {
    Command cmd(c);

    int r = 0, g = 0, b = 0;
    if (!parseByteArg(cmd, "red", r)) return false;
    if (!parseByteArg(cmd, "green", g)) return false;
    if (!parseByteArg(cmd, "blue", b)) return false;

    applyLedColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);
    return true;
}

// e.g. "led hex ff00ff"
static uint32_t ledHexCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("value");
    String strValue = arg.getValue();
    strValue.trim();
    if (strValue.startsWith("#")) strValue = strValue.substring(1);

    char *end = nullptr;
    uint32_t value = strtoul(strValue.c_str(), &end, 16);

    if (strValue.length() == 0 || *end != '\0' || value > 0xFFFFFF) {
        serialDevice->println("Invalid color: " + strValue);
        return false;
    }

    applyLedColor(value);
    return true;
}

// e.g. "led br 50" (0-100, same scale as the settings menu)
static uint32_t ledBrightnessCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("value");
    String strValue = arg.getValue();
    strValue.trim();

    int value = atoi(strValue.c_str());
    if (value < 0 || value > 100) {
        serialDevice->println("Invalid value: " + strValue + " (expected 0-100)");
        return false;
    }

    bruceConfig.setLedBright(value);
    setLedBrightness(value);
    serialDevice->println("LED brightness set to " + String(value) + "%");
    return true;
}

// e.g. "led effect 3"
static uint32_t ledEffectCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("value");
    String strValue = arg.getValue();
    strValue.trim();

    int value = atoi(strValue.c_str());
    if (value < LED_EFFECT_SOLID || value > LED_EFFECT_FIRE) {
        serialDevice->println("Invalid effect: " + strValue + " (expected 0-9)");
        return false;
    }

    bruceConfig.setLedEffect(value);
    ledSetup();
    serialDevice->println("LED effect set to " + String(value));
    return true;
}

// Same as picking "OFF" in the LED color menu.
static uint32_t ledOffCallback(cmd *c) {
    applyLedColor(0);
    return true;
}

void createLedCommands(SimpleCLI *cli) {
    Command ledCmd = cli->addCompositeCmd("led");

    Command redCmd = ledCmd.addCommand("r/ed", ledRedCallback);
    redCmd.addPosArg("value");
    Command greenCmd = ledCmd.addCommand("g/reen", ledGreenCallback);
    greenCmd.addPosArg("value");
    Command blueCmd = ledCmd.addCommand("b/lue", ledBlueCallback);
    blueCmd.addPosArg("value");

    Command rgbCmd = ledCmd.addCommand("rgb", ledRgbCallback);
    rgbCmd.addPosArg("red");
    rgbCmd.addPosArg("green");
    rgbCmd.addPosArg("blue");

    Command hexCmd = ledCmd.addCommand("hex", ledHexCallback);
    hexCmd.addPosArg("value");

    Command brightCmd = ledCmd.addCommand("br/ight/ness", ledBrightnessCallback);
    brightCmd.addPosArg("value");

    Command effectCmd = ledCmd.addCommand("effect", ledEffectCallback);
    effectCmd.addPosArg("value");

    ledCmd.addCommand("off", ledOffCallback);
}

#else
void createLedCommands(SimpleCLI *cli) {}
#endif
