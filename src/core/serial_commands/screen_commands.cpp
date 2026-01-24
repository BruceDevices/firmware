#include "screen_commands.h"
#include "core/settings.h"
#include "core/utils.h" // time
#include <globals.h>

uint32_t brightnessCallback(cmd *c) {
    // backlight brightness adjust (range 0-255) https://docs.flipper.net/development/cli/#XQQAI
    // e.g. "led br 127"

    Command cmd(c);

    Argument arg = cmd.getArgument("value");
    String strValue = arg.getValue();
    strValue.trim();

    int value = (atoi(strValue.c_str()) * 100) / 255; // convert to 0-100 range
    value = min(max(1, value), 100);

    serialDevice->println("Settings led brightness to " + String(value) + "%");
    setBrightness(value, false);
    return true;
}

uint32_t rgbColorCallback(cmd *c) {
    // change UI color
    // e.g. "screen color rgb 255 255 255"

    Command cmd(c);

    Argument redArg = cmd.getArgument("red");
    Argument greenArg = cmd.getArgument("green");
    Argument blueArg = cmd.getArgument("blue");
    String strRed = redArg.getValue();
    String strGreen = greenArg.getValue();
    String strBlue = blueArg.getValue();
    strRed.trim();
    strGreen.trim();
    strBlue.trim();

    int r = atoi(strRed.c_str());
    int g = atoi(strGreen.c_str());
    int b = atoi(strBlue.c_str());

    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        serialDevice->println("Invalid color: " + strRed + " " + strGreen + " " + strBlue);
        return false;
    }

    uint16_t hexColor = tft.color565(r, g, b);
    bruceConfig.priColor = hexColor; // change global var, dont save in config
    return true;
}

uint32_t hexColorCallback(cmd *c) {
    // change UI color
    // e.g. "screen color hex ff00ff"

    Command cmd(c);

    Argument arg = cmd.getArgument("value");
    String strValue = arg.getValue();
    strValue.trim();

    if (strValue.length() % 2 == 1) {
        serialDevice->println("Invalid hex value: " + strValue);
        return false;
    }

    uint32_t value = static_cast<uint32_t>(std::stoul(strValue.c_str(), nullptr, 16));

    if (value > 0xFFFFFF) {
        serialDevice->println("Invalid color: " + strValue);
        return false;
    }

    uint16_t r = (value >> 8) & 0xF800;
    uint16_t g = (value >> 5) & 0x07E0;
    uint16_t b = (value >> 3) & 0x001F;

    uint16_t hexColor = (r | g | b);
    bruceConfig.priColor = hexColor; // change global var, dont save in config
    return true;
}

uint32_t clockCallback(cmd *c) {
#if defined(HAS_RTC)
    updateTimeStr(_rtc.getTimeStruct());
    serialDevice->printf("\nCurrent time: %s", timeStr);
#else
    updateTimeStr(rtc.getTimeStruct());
    serialDevice->printf("\nCurrent time: %s", timeStr);
#endif
    return true;
}

void createScreenCommands(SimpleCLI *cli) {
    Command clockCmd = cli->addCommand("clock", clockCallback);

    Command screenCmd = cli->addCompositeCmd("screen");

    Command brightCmd = screenCmd.addCommand("br/ight/ness", brightnessCallback);
    brightCmd.addPosArg("value");

    Command colorCmd = screenCmd.addCompositeCmd("color");
    Command rgbColorCmd = colorCmd.addCommand("rgb", rgbColorCallback);
    rgbColorCmd.addPosArg("red");
    rgbColorCmd.addPosArg("green");
    rgbColorCmd.addPosArg("blue");
    Command hexColorCmd = colorCmd.addCommand("hex", hexColorCallback);
    hexColorCmd.addPosArg("value");
}
