#include "ir_utils.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include <algorithm>

void setup_ir_pin(int pin, uint8_t mode) {
    if (bruceConfigPins.SDCARD_bus.checkConflict(pin)) sdcardSPI.end();
    gpio_reset_pin((gpio_num_t)pin);
    pinMode(pin, mode);
}

namespace {

const char *DIR_PICKER_CHOOSE_LABEL = ">> Choose current folder";
const char *DIR_PICKER_BACK_LABEL = ">> Back";

void readDirsOnly(FS &fs, const String &folder, std::vector<FileList> &out) {
    out.clear();

    FileList chooseEntry;
    chooseEntry.filename = DIR_PICKER_CHOOSE_LABEL;
    chooseEntry.folder = false;
    chooseEntry.operation = true; // reuses the "operation" flag; color is overridden in drawDirList()
    out.push_back(chooseEntry);

    File root = fs.open(folder);
    if (root && root.isDirectory()) {
        std::vector<FileList> dirs;
        while (true) {
            bool isDir;
            String fullPath = root.getNextFileName(&isDir);
            if (fullPath == "") break;
            if (!isDir) continue; // directories only, files are never shown/loaded
            FileList item;
            item.filename = fullPath.substring(fullPath.lastIndexOf('/') + 1);
            item.folder = true;
            item.operation = false;
            dirs.push_back(item);
        }
        root.close();
        std::sort(dirs.begin(), dirs.end(), sortList);
        for (auto &d : dirs) out.push_back(d);
    }

    FileList backEntry;
    backEntry.filename = DIR_PICKER_BACK_LABEL;
    backEntry.folder = false;
    backEntry.operation = true;
    out.push_back(backEntry);
}

// Small renderer, mirroring the look of core listFiles() but with a
// current-path header and a green highlight for the "choose current folder" entry.

Opt_Coord drawDirList(int index, const std::vector<FileList> &list, const String &curPath) {
    Opt_Coord coord;
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    if (index == 0) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
    }

    // Header: current path, right-truncated so it always fits one line
    tft.setTextSize(FP);
    int headerChars = (tftWidth - 20) / (6 * FP);
    String hdr = curPath;
    if ((int)hdr.length() > headerChars) hdr = "..." + hdr.substring(hdr.length() - (headerChars - 3));
    tft.fillRect(8, 8, tftWidth - 16, FP * LH, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(hdr, 10, 8, 1);

    int listTop = 8 + FP * LH + 4;
    tft.setCursor(10, listTop);
    tft.setTextSize(FM);

    int arraySize = (int)list.size();
    int maxItems = (tftHeight - listTop - 10) / (LH * FM);
    if (maxItems < 1) maxItems = 1;
    int start = 0;
    if (index >= maxItems) start = index - maxItems + 1;
    int nchars = (tftWidth - 20) / (6 * FM);

    for (int i = start; i < arraySize && i < start + maxItems; i++) {
        tft.setCursor(10, tft.getCursorY());

        bool isChoose = (list[i].filename == DIR_PICKER_CHOOSE_LABEL);
        uint16_t color =
            isChoose ? TFT_GREEN : (list[i].operation ? ALCOLOR : getColorVariation(bruceConfig.priColor));
        tft.setTextColor(color, bruceConfig.bgColor);

        String txt;
        if (index == i) {
            txt = ">";
            coord.x = 10 + FM * LW;
            coord.y = tft.getCursorY();
            coord.size = nchars;
            coord.fgcolor = color;
            coord.bgcolor = bruceConfig.bgColor;
        } else {
            txt = " ";
        }
        txt += list[i].filename + "                 ";
        tft.println(txt.substring(0, nchars));
    }
    return coord;
}

} // namespace

String pickDirectory(FS &fs, String rootPath) {
    if (!fs.exists(rootPath)) rootPath = "/";
    if (!fs.exists(rootPath)) return "";

    if (&fs == &SD) {
        if (!setupSdCard()) {
            displayError("Fail Mounting SD", true);
            return "";
        }
    }

    tft.drawPixel(0, 0, 0);
    tft.fillScreen(bruceConfig.bgColor);
    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);

    String folder = rootPath;
    std::vector<FileList> dirList;
    readDirsOnly(fs, folder, dirList);

    int index = 0;
    bool redraw = true;
    Opt_Coord coord;

    while (true) {
        delay(10);
        if (redraw) {
            coord = drawDirList(index, dirList, folder);
            redraw = false;
        }
        displayScrollingText(dirList[index].filename, coord);

        if (EscPress && PrevPress) EscPress = false;

        if (check(EscPress)) {
            if (folder == "/") return ""; // cancelled at root
            folder = folder.substring(0, folder.lastIndexOf('/'));
            if (folder == "") folder = "/";
            readDirsOnly(fs, folder, dirList);
            index = 0;
            redraw = true;
            continue;
        }
        if (check(PrevPress) || check(UpPress)) {
            index = (index == 0) ? (int)dirList.size() - 1 : index - 1;
            redraw = true;
        }
        if (check(NextPress) || check(DownPress)) {
            index = (index == (int)dirList.size() - 1) ? 0 : index + 1;
            redraw = true;
        }
        if (check(SelPress)) {
            const String &sel = dirList[index].filename;
            while (check(SelPress)) yield(); // debounce, avoid double-activation

            if (sel == DIR_PICKER_CHOOSE_LABEL) {
                return folder;
            } else if (sel == DIR_PICKER_BACK_LABEL) {
                if (folder == "/") return ""; // cancelled at root
                folder = folder.substring(0, folder.lastIndexOf('/'));
                if (folder == "") folder = "/";
            } else {
                // drill into subfolder
                folder = folder + (folder == "/" ? "" : "/") + sel;
            }
            readDirsOnly(fs, folder, dirList);
            index = 0;
            redraw = true;
        }
    }
}
