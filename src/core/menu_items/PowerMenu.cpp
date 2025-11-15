#include "PowerMenu.h"
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "core/utils.h"

void PowerMenu::optionsMenu() {
    options = {
        {"Sleep",                  setSleepMode               },
        {"Deep Sleep",             goToDeepSleep              },
        {"Power Off",              powerOff                   },
        {"Dim Timeout",            setDimmerTimeMenu          },
        {"Screen Off Timeout",     setScreenOffTimeoutMenu    },
        {"Auto Sleep Timeout",     setAutoSleepTimeoutMenu    },
        {"Auto Deep Sleep Timeout", setAutoDeepSleepTimeoutMenu},
        {"Battery Log Interval",   setBatteryLogIntervalMenu  },
        {"View Battery Log (Text)", showBatteryLogText        },
        {"View Battery Log (Graph)", showBatteryLogGraph      },
        {"Delete Battery Log",     deleteBatteryLogFileMenu   },
    };

    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Power");
}

void PowerMenu::drawIcon(float scale) {
    clearIconArea();

    int arcThickness = scale * 6;
    if (arcThickness < 4) arcThickness = 4;
    int outerRadius = scale * 45;
    if (outerRadius > iconAreaH / 2 - 10) outerRadius = iconAreaH / 2 - 10;
    if (outerRadius < arcThickness + 6) outerRadius = arcThickness + 6;
    int innerRadius = outerRadius - arcThickness;
    int offsetY = scale * 6;

    tft.drawArc(
        iconCenterX,
        iconCenterY + offsetY,
        outerRadius,
        innerRadius,
        40,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor,
        true
    );

    int lineTop = iconCenterY - outerRadius - arcThickness;
    int lineBottom = iconCenterY + offsetY - innerRadius / 2;
    tft.drawWideLine(
        iconCenterX,
        lineTop,
        iconCenterX,
        lineBottom,
        arcThickness,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );

    int pulseRadius = arcThickness / 2 + 2;
    tft.fillCircle(iconCenterX, lineTop + arcThickness * 2, pulseRadius, bruceConfig.priColor);
}

void PowerMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(),
        bruceConfig.getThemeItemImg(bruceConfig.theme.paths.power),
        0,
        imgCenterY,
        true
    );
}
