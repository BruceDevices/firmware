#ifndef __MENU_ITEM_INTERFACE_H__
#define __MENU_ITEM_INTERFACE_H__

#include "core/display.h"
#include <globals.h>

class MenuItemInterface {
public:
    virtual ~MenuItemInterface() = default;
    virtual void optionsMenu(void) = 0;
    virtual void drawIcon(float scale = 1) = 0;
    virtual void drawIconImg() {
        drawImg(
            *bruceConfig.themeFS(),
            bruceConfig.getThemeItemImg(themePath()),  // const String& - no heap alloc
            0,
            imgCenterY,
            true,
            bruceConfig.theme.gifDuration,
            false
        );
    }
    virtual bool hasTheme() = 0;
    virtual const String& themePath() = 0;

    bool checkTheme() { return hasTheme() && themePath().length() > 0; }
    String getName() const { return String(_name); }

    void draw(float scale = 1) {
        if (rotation != bruceConfigPins.rotation) resetCoordinates();
        if (!checkTheme()) {
            tft.fillRect(0, 27, tftWidth, tftHeight - 27, bruceConfig.bgColor);
            drawIcon(scale);
            drawArrows(scale);
            drawTitle(scale);
        } else {
            if (bruceConfig.theme.label)
                drawTitle(scale); // If using .GIF, labels are draw after complete, which takes some time
            drawIconImg();
            if (bruceConfig.theme.label) drawTitle(scale); // Makes sure to draw over the image
        }
        drawStatusBar();
    }

    void drawArrows(float scale = 1) {
        tft.fillRect(arrowAreaX, iconAreaY, arrowAreaW, iconAreaH, bruceConfig.bgColor);
        tft.fillRect(
            tftWidth - arrowAreaX - arrowAreaW, iconAreaY, arrowAreaW, iconAreaH, bruceConfig.bgColor
        );

        int arrowSize = scale * 10;
        int lineWidth = scale * 3;

        int arrowX = BORDER_PAD_X + 1.5 * arrowSize;
        int arrowY = iconCenterY + 1.5 * arrowSize;

        // Left Arrow
        tft.drawWideLine(
            arrowX,
            arrowY,
            arrowX + arrowSize,
            arrowY + arrowSize,
            lineWidth,
            bruceConfig.priColor,
            bruceConfig.bgColor
        );
        tft.drawWideLine(
            arrowX,
            arrowY,
            arrowX + arrowSize,
            arrowY - arrowSize,
            lineWidth,
            bruceConfig.priColor,
            bruceConfig.bgColor
        );

        // Right Arrow
        tft.drawWideLine(
            tftWidth - arrowX,
            arrowY,
            tftWidth - arrowX - arrowSize,
            arrowY + arrowSize,
            lineWidth,
            bruceConfig.priColor,
            bruceConfig.bgColor
        );
        tft.drawWideLine(
            tftWidth - arrowX,
            arrowY,
            tftWidth - arrowX - arrowSize,
            arrowY - arrowSize,
            lineWidth,
            bruceConfig.priColor,
            bruceConfig.bgColor
        );
    }

    void drawTitle(float scale = 1) {
        // Title sits in the gap between the icon area and the footer.
        // Use FG (size 3) so it's readable on large screens.
        const int fontSize = FG;
        const int textH    = LH * fontSize;
        int titleY = iconAreaY + iconAreaH + 4;   // 4 px below icon area

        tft.setTextSize(fontSize);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.fillRect(BORDER_PAD_X, titleY, tftWidth - 2 * BORDER_PAD_X, textH + 2, bruceConfig.bgColor);
        int nchars = (tftWidth - 2 * BORDER_PAD_X) / (LW * fontSize);
        tft.drawCentreString(getName().substring(0, nchars), iconCenterX, titleY, SMOOTH_FONT);
    }

protected:
    const char *_name = "";
    uint8_t rotation = ROTATION;

    // These are updated by resetCoordinates() before first use.
    int iconAreaH   = 0;
    int iconAreaW   = 0;
    int iconCenterX = 0;
    int iconCenterY = 0;
    int imgCenterY  = 13;
    int iconAreaX   = 0;
    int iconAreaY   = 0;
    int arrowAreaX  = BORDER_PAD_X;
    int arrowAreaW  = 0;

    MenuItemInterface(const char *name) : _name(name) {}

    void clearIconArea(void) {
        tft.fillRect(iconAreaX, iconAreaY, iconAreaW, iconAreaH, bruceConfig.bgColor);
    }
    void clearImgArea(void) { tft.fillRect(7, 27, tftWidth - 14, tftHeight - 34, bruceConfig.bgColor); }

    void resetCoordinates(void) {
        // On touch screens the bottom of the content area is above the footer
        // (54 px) and the menu item title (FG*LH+6 = 30 px).
        // On non-touch screens use the original symmetric padding.
        int topY    = BORDER_PAD_Y;          // below status bar
#if defined(HAS_TOUCH)
        int bottomY = tftHeight - 54 - 30;   // above footer + title row
#else
        int bottomY = tftHeight - BORDER_PAD_Y;
#endif
        int availH  = bottomY - topY;

        // Keep the icon square; in landscape constrain by height, in portrait by width.
        int side = (tftWidth > tftHeight) ? availH : (tftWidth - 2 * BORDER_PAD_Y);
        if (side % 2 != 0) side--;           // keep even for clean centering
        iconAreaH = side;
        iconAreaW = side;

        iconCenterX = tftWidth / 2;
        iconCenterY = topY + side / 2;       // centred in the available content band

        iconAreaX = iconCenterX - side / 2;
        iconAreaY = iconCenterY - side / 2;

        arrowAreaX = BORDER_PAD_X;
        arrowAreaW = max(1, iconAreaX - arrowAreaX);

        rotation = bruceConfigPins.rotation;
    }

private:
};

#endif // __MENU_ITEM_INTERFACE_H__
