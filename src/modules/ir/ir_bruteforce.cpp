/**
 * @file ir_bruteforce.cpp
 * @brief IR bruteforce module for Bruce firmware by 7wp81x.
 *
 * Scans a directory recursively for *.ir files, either filtering
 * signals by keyword (e.g. "power", "on", "off") or - in "All IR
 * files" mode - taking every signal found, then blasts them one file
 * at a time so the user can observe which remote controls their
 * device.
 *
 * Navigation (Cardputer):
 *   =  / right   Next file
 *   <  / left    Previous file
 *   Enter        Pause / Resume (persists across Next/Prev and auto-advance)
 *   o            Previous signal in current file
 *   p            Next signal in current file
 *   s            Send current signal right now (does not wait for auto delay)
 *   m            Mark current file as match
 *   l            Toggle "loop current file" (stay on this file forever
 *                instead of auto-advancing once all cycles finish)
 *   ` or Esc     Save matches = /BruceIR/Bruteforce/match_<date>_<hhmmss>.sc and exit
 *
 * Memory notes:
 *   The directory scan only keeps the *path* of every file that has at
 *   least one matching signal (plus a small match count for the UI) -
 *   it never stores the parsed signal data for more than one file at a
 *   time. The signals of the file currently being blasted are the only
 *   ones held in RAM, and are (re)loaded on demand whenever the user
 *   moves to a different file. This keeps memory use flat regardless of
 *   how many .ir files/signals exist under the chosen directory, which
 *   is what used to crash on Cardputer/Advanced when pointed at a large
 *   IR database. A hard cap on the number of matched files, and a
 *   free-heap check, also stop the scan itself from running the device
 *   out of memory on very large trees.
 */

#if !defined(LITE_VERSION)
#include "ir_bruteforce.h"
#include "custom_ir.h"
#include "TV-B-Gone.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "ir_utils.h"
#include <SD.h>
#include <vector>

#define IRBRF_MAX_FILES 500
#define IRBRF_MIN_FREE_HEAP (40 * 1024)

struct IrBrfSignal {
    String name;
    String type;
    String protocol;
    String address;
    String command;
    String data;
    uint16_t frequency = 38000;
    uint8_t bits = 32;
};

struct IrBrfFile {
    String path;
    int matchCount = 0;
};

static void irbrf_collectFiles(FS *fs, std::vector<String> &dirStack,
                               const std::vector<String> &keywords,
                               std::vector<IrBrfFile> &out,
                               int &scannedFiles, bool &heapLimited,
                               bool &cancelled);
static int irbrf_countMatches(FS *fs, const String &path,
                              const std::vector<String> &keywords);
static void irbrf_loadFileSignals(FS *fs, const String &path,
                                  const std::vector<String> &keywords,
                                  std::vector<IrBrfSignal> &out);
static void irbrf_sendSignal(const IrBrfSignal &sig);
static void irbrf_drawStatic(const String &filePath, int fileIdx, int fileTotal);
static void irbrf_drawDynamic(int sigIdx, int sigTotal, int cycle, int maxCycles,
                              bool paused, const String &status, int matchCount,
                              bool loopFile);
static bool irbrf_interruptibleDelay(uint32_t ms);
static String irbrf_saveMatches(const std::vector<String> &matches);
static void irbrf_showFinishScreen(const String &savedPath);
static std::vector<String> irbrf_splitKeywords(const String &csv);
static String irbrf_shortPath(const String &path);

/*
 * irbrf_nameMatches
 * Checks if a signal name matches any of the given keywords.
 * Matching is case insensitive and uses substring search, not exact match.
 * If the keywords list is empty, this always returns true, which is how
 * "All IR files" mode accepts every signal without filtering.
 */
static bool irbrf_nameMatches(const String &name,
                              const std::vector<String> &keywords) {
    if (keywords.empty()) return true;
    String lower = name;
    lower.toLowerCase();
    for (const auto &kw : keywords) {
        String k = kw;
        k.trim();
        k.toLowerCase();
        if (k.length() > 0 && lower.indexOf(k) >= 0) return true;
    }
    return false;
}

/*
 * irbrf_collectFiles
 * Walks the directory tree starting from the paths already pushed into
 * dirStack, using an explicit stack instead of recursion to avoid deep
 * call stacks on large trees.
 *
 * For every .ir file found, it calls irbrf_countMatches to see how many
 * signals inside match the keyword filter, and only keeps the file path
 * plus the match count in "out". The actual signal data is not loaded
 * here, which keeps memory usage flat no matter how many files exist.
 *
 * The scan can stop early for three reasons, each reported back through
 * an out parameter so the caller can react accordingly:
 *   - out reaches IRBRF_MAX_FILES (hard cap)
 *   - free heap drops below IRBRF_MIN_FREE_HEAP (heapLimited = true)
 *   - the user presses Esc mid scan (cancelled = true)
 *
 * On cancel or heap limit, dirStack is left as is so scanning can be
 * resumed later by calling this function again with the same stack.
 */
static void irbrf_collectFiles(FS *fs, std::vector<String> &dirStack,
                               const std::vector<String> &keywords,
                               std::vector<IrBrfFile> &out,
                               int &scannedFiles, bool &heapLimited,
                               bool &cancelled) {
    cancelled = false;

    while (!dirStack.empty()) {
        if ((int)out.size() >= IRBRF_MAX_FILES) return;
        if (ESP.getFreeHeap() < IRBRF_MIN_FREE_HEAP) {
            heapLimited = true;
            return;
        }
        if (check(EscPress)) {
            cancelled = true;
            return;
        }

        String dir = dirStack.back();
        dirStack.pop_back();

        File root = fs->open(dir);
        if (!root || !root.isDirectory()) continue;

        File entry = root.openNextFile();
        while (entry) {
            if ((int)out.size() >= IRBRF_MAX_FILES) { entry.close(); root.close(); return; }
            if (ESP.getFreeHeap() < IRBRF_MIN_FREE_HEAP) {
                heapLimited = true;
                entry.close();
                root.close();
                return;
            }
            if (check(EscPress)) {
                cancelled = true;
                entry.close();
                root.close();
                return;
            }

            String entryPath = String(entry.path());
            bool isDir = entry.isDirectory();
            entry.close();

            if (isDir) {
                dirStack.push_back(entryPath);
            } else if (entryPath.endsWith(".ir") || entryPath.endsWith(".IR")) {
                scannedFiles++;
                int n = irbrf_countMatches(fs, entryPath, keywords);
                if (n > 0) {
                    IrBrfFile bf;
                    bf.path = entryPath;
                    bf.matchCount = n;
                    out.push_back(bf);
                }
                if (scannedFiles % 25 == 0) {
                    tft.setTextSize(FP);
                    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                    tft.fillRect(10, tftHeight / 2 + 12, tftWidth - 20, FP * LH, bruceConfig.bgColor);
                    String prog = "Scanned " + String(scannedFiles) + ", matched " + String(out.size()) +
                                  "  (Esc=pause)";
                    tft.drawCentreString(prog, tftWidth / 2, tftHeight / 2 + 12, 1);
                }
            }

            entry = root.openNextFile();
        }
        root.close();
    }
}

/*
 * irbrf_countMatches
 * Opens a single .ir file and counts how many "name:" lines match the
 * keyword filter, without storing any signal data. This is a cheap,
 * read only pass used during the scan phase (irbrf_collectFiles) so
 * only the count is known, the file is parsed again in full later by
 * irbrf_loadFileSignals only if the user actually reaches that file.
 */
static int irbrf_countMatches(FS *fs, const String &path,
                              const std::vector<String> &keywords) {
    File f = fs->open(path, FILE_READ);
    if (!f) return 0;

    int count = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        line.trim();
        if (line.startsWith("name:")) {
            String name = line.substring(5);
            name.trim();
            if (irbrf_nameMatches(name, keywords)) count++;
        }
    }
    f.close();
    return count;
}

/*
 * irbrf_loadFileSignals
 * Fully parses one .ir file into a list of IrBrfSignal entries, keeping
 * only the signals whose name matches the keyword filter.
 *
 * The file format is treated as a series of blocks, each block starts
 * with a "name:" line and ends at the next blank line, comment line, or
 * the next "name:" line. Recognized fields per block are type, protocol,
 * address, command, frequency, bits, and data (data/value/state are all
 * treated as the same field).
 *
 * This is only called for the file the user is currently on, never for
 * the whole matched list at once, which is what keeps RAM usage low
 * even when hundreds of files are queued up.
 */
static void irbrf_loadFileSignals(FS *fs, const String &path,
                                  const std::vector<String> &keywords,
                                  std::vector<IrBrfSignal> &out) {
    out.clear();
    File f = fs->open(path, FILE_READ);
    if (!f) return;

    IrBrfSignal cur;
    bool inBlock = false;

    auto flushBlock = [&]() {
        if (!inBlock) return;
        if (irbrf_nameMatches(cur.name, keywords)) out.push_back(cur);
        cur = IrBrfSignal();
        inBlock = false;
    };

    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        line.trim();

        if (line.startsWith("#") || line.length() == 0) {
            flushBlock();
            continue;
        }

        if (line.startsWith("name:")) {
            flushBlock();
            cur.name = line.substring(5); cur.name.trim();
            inBlock = true;
        } else if (line.startsWith("type:")) {
            cur.type = line.substring(5); cur.type.trim();
        } else if (line.startsWith("protocol:")) {
            cur.protocol = line.substring(9); cur.protocol.trim();
        } else if (line.startsWith("address:")) {
            cur.address = line.substring(8); cur.address.trim();
        } else if (line.startsWith("command:")) {
            cur.command = line.substring(8); cur.command.trim();
        } else if (line.startsWith("frequency:")) {
            cur.frequency = (uint16_t)line.substring(10).toInt();
        } else if (line.startsWith("bits:")) {
            cur.bits = (uint8_t)line.substring(5).toInt();
        } else if (line.startsWith("data:") ||
                   line.startsWith("value:") ||
                   line.startsWith("state:")) {
            int colon = line.indexOf(':');
            cur.data = line.substring(colon + 1); cur.data.trim();
        }
    }
    flushBlock();
    f.close();
}

/*
 * irbrf_sendSignal
 * Converts an IrBrfSignal into the IRCode struct expected by
 * sendIRCommand and transmits it. hideDefaultUI is set to true because
 * this module draws its own status screen instead of using the default
 * one shot IR transmit UI.
 */
static void irbrf_sendSignal(const IrBrfSignal &sig) {
    IRCode code;
    code.name     = sig.name;
    code.type     = sig.type;
    code.protocol = sig.protocol;
    code.address  = sig.address;
    code.command  = sig.command;
    code.data     = sig.data;
    code.frequency = sig.frequency;
    code.bits     = sig.bits;
    sendIRCommand(&code, /*hideDefaultUI=*/true);
}

/*
 * irbrf_shortPath
 * Returns a shortened version of a full file path for display purposes,
 * showing only the parent folder and file name instead of the full
 * absolute path, for example "/IR/TV/samsung.ir" becomes "TV/samsung.ir".
 * Falls back to returning the original path if it has no slashes.
 */
static String irbrf_shortPath(const String &path) {
    int last  = path.lastIndexOf('/');
    if (last <= 0) return path;
    int prev = path.lastIndexOf('/', last - 1);
    return (prev >= 0) ? path.substring(prev + 1) : path.substring(last + 1);
}

/*
 * Screen layout state shared between irbrf_drawStatic and
 * irbrf_drawDynamic. irbrf_drawStatic computes these y and x positions
 * once per file so irbrf_drawDynamic can redraw just the changing
 * regions (progress info, cycle count, current signal name) without
 * recalculating layout or redrawing the whole screen every tick.
 */
namespace {
    int g_yFileHeader;
    int g_headerRightX;
    int g_yProgressBar;
    int g_yFileName;
    int g_yCycleLine;
    int g_ySignalName;
}

/*
 * irbrf_drawStatic
 * Draws the parts of the screen that only need to be drawn once per
 * file: the title border, "File X/Y" header, file progress bar, the
 * short file name, and the footer with key hints. Also stores the
 * screen coordinates used by irbrf_drawDynamic into the g_y* globals
 * above.
 */
static void irbrf_drawStatic(const String &filePath, int fileIdx, int fileTotal) {
    drawMainBorderWithTitle("IR Bruteforce");

    int y = BORDER_PAD_Y + FM * LH + 6;
    tft.setTextSize(FP);

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    char header[24];
    snprintf(header, sizeof(header), "File %d/%d", fileIdx + 1, fileTotal);
    g_yFileHeader = y;
    tft.drawString(header, 10, y, 1);

    g_headerRightX = 10 + 13 * 6 * FP;
    if (g_headerRightX > tftWidth - 40) g_headerRightX = tftWidth - 40;

    int barX = 10, barY = y + 10, barW = tftWidth - 20, barH = 5;
    g_yProgressBar = barY;
    tft.fillRect(barX, barY, barW, barH, TFT_DARKGREY);
    int filled = (fileTotal > 1) ? (barW * fileIdx / (fileTotal - 1)) : barW;
    tft.fillRect(barX, barY, filled, barH, TFT_GREEN);

    y += 20;

    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    String shortName = irbrf_shortPath(filePath);
    g_yFileName = y;
    tft.drawString(shortName, 10, y, 1);

    y += 10;
    g_yCycleLine = y;

    y += 10;
    g_ySignalName = y;

    int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
#ifdef HAS_KEYBOARD
    tft.drawCentreString("< > Ent] O/P=Sig S]end M]atch L]oop",
                         tftWidth / 2, footerY, 1);
#else
    // No physical keys on this board, so "m" and "l" do not apply here;
    // those actions are reached through the Esc menu instead.
    tft.drawCentreString("<> Prev/Next Sel=Pause Esc=Menu",
                         tftWidth / 2, footerY, 1);
#endif
}

/*
 * irbrf_drawDynamic
 * Redraws only the parts of the screen that change while a file is
 * being blasted: matched count, AUTO/LOOP mode label, cycle and signal
 * counters, and the status line (which shows the currently transmitted
 * signal name, or "[PAUSED] ..." when paused). Relies on the layout
 * positions computed earlier by irbrf_drawStatic, so it must be called
 * after that function has run at least once for the current file.
 */
static void irbrf_drawDynamic(int sigIdx, int sigTotal, int cycle, int maxCycles,
                              bool paused, const String &status, int matchCount,
                              bool loopFile) {
    tft.setTextSize(FP);
    int rightW = tftWidth - g_headerRightX - 5;
    if (rightW > 0) {
        tft.fillRect(g_headerRightX, g_yFileHeader, rightW, FP * LH, bruceConfig.bgColor);

        char matchBuf[16];
        snprintf(matchBuf, sizeof(matchBuf), "Matched: %d", matchCount);
        String modeLabel = loopFile ? "LOOP" : "AUTO";
        uint16_t modeColor = loopFile ? TFT_GREEN : TFT_DARKGREY;

        int modeW  = modeLabel.length() * 6 * FP;
        int modeX  = tftWidth - 10 - modeW;
        int matchW = (int)String(matchBuf).length() * 6 * FP;
        int matchX = modeX - 8 - matchW;
        if (matchX < g_headerRightX) matchX = g_headerRightX;

        tft.setTextColor(matchCount > 0 ? TFT_GREEN : TFT_DARKGREY, bruceConfig.bgColor);
        tft.drawString(matchBuf, matchX, g_yFileHeader, 1);

        tft.setTextColor(modeColor, bruceConfig.bgColor);
        tft.drawString(modeLabel, modeX, g_yFileHeader, 1);
    }

    tft.fillRect(10, g_yCycleLine, tftWidth - 20, FP * LH, bruceConfig.bgColor);
    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    char cycleBuf[24];
    snprintf(cycleBuf, sizeof(cycleBuf), "Cycle %d/%d  Sig %d/%d",
             cycle, maxCycles, sigIdx + 1, sigTotal);
    tft.drawString(cycleBuf, 10, g_yCycleLine, 1);

    tft.fillRect(10, g_ySignalName, tftWidth - 20, FP * LH, bruceConfig.bgColor);
    tft.setTextColor(paused ? TFT_YELLOW : TFT_GREEN, bruceConfig.bgColor);
    String st = paused ? String("[PAUSED] ") + status : status;
    int maxChars = (tftWidth - 20) / (6 * FP);
    if ((int)st.length() > maxChars) st = st.substring(0, maxChars);
    tft.drawString(st, 10, g_ySignalName, 1);
}

/*
 * irbrf_interruptibleDelay
 * Waits for the given number of milliseconds, but returns early (with
 * false) if the user presses Esc, Next, Prev, or Select during the
 * wait, so the gap between transmitted signals does not feel like the
 * device is unresponsive. Returns true if the full delay completed
 * without any key press.
 */
static bool irbrf_interruptibleDelay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
        if (EscPress || NextPress || PrevPress || SelPress) return false;
        delay(10);
    }
    return true;
}


/*
 * irbrf_saveMatches
 * Writes the list of file paths the user marked as a match (pressed
 * "m" on) into a timestamped text file under /BruceIR/Bruteforce on the SD card.
 * Creates the /BruceIR folder if it does not exist yet. If the clock
 * has not been set, falls back to using millis() in the file name so
 * the file is still unique. Returns the saved file path, or an empty
 * string if there was nothing to save or the file could not be opened.
 */
static String irbrf_saveMatches(const std::vector<String> &matches) {
    if (matches.empty()) return "";

    if (!SD.exists("/BruceIR")) SD.mkdir("/BruceIR");
    if (!SD.exists("/BruceIR/Bruteforce")) SD.mkdir("/BruceIR/Bruteforce");
    char fname[64];
    if (clock_set && timeInfo != nullptr) {
        char stamp[24];
        strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", timeInfo);
        snprintf(fname, sizeof(fname), "/BruceIR/Bruteforce/match_%s.sc", stamp);
    } else {
        snprintf(fname, sizeof(fname), "/BruceIR/Bruteforce/match_%lu.sc", (unsigned long)millis());
    }

    File out = SD.open(fname, FILE_WRITE);
    if (!out) {
        displayError("Cannot write results", true);
        return "";
    }
    out.println("# IR Bruteforce Matches");
    out.println("# Generated by Bruce firmware");
    out.println("#");
    for (const auto &m : matches) {
        out.println(m);
    }
    out.close();

    return String(fname);
}

/*
 * irbrf_showFinishScreen
 * Displays the final results screen after the scan/blast session ends,
 * showing either the path where matches were saved, or a "no matches
 * saved" message. Waits for any key press before returning, with a
 * small guard loop first to drain any key that is still being held
 * down from the previous screen.
 */
static void irbrf_showFinishScreen(const String &savedPath) {
    drawMainBorderWithTitle("IR Bruteforce");

    int y = tftHeight / 2 - 20;
    tft.setTextSize(FP);

    if (savedPath.length() > 0) {
        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        tft.drawCentreString("Match list saved:", tftWidth / 2, y, 1);
        y += FP * LH + 4;
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        tft.drawCentreString(savedPath, tftWidth / 2, y, 1);
    } else {
        tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        tft.drawCentreString("No matched saved", tftWidth / 2, y, 1);
    }

    y = tftHeight - BORDER_PAD_X - FP * LH - 4;
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("Press any key to continue", tftWidth / 2, y, 1);

    while (check(EscPress) || check(NextPress) || check(PrevPress) ||
           check(SelPress) || check(UpPress) || check(DownPress)) yield();

    while (!(check(EscPress) || check(NextPress) || check(PrevPress) ||
             check(SelPress) || check(UpPress) || check(DownPress))) {
        delay(20);
    }
}

/*
 * irbrf_splitKeywords
 * Splits a comma separated keyword string typed by the user (for
 * example "power,on,off") into a trimmed vector of individual
 * keywords. Empty tokens caused by extra commas are skipped.
 */
static std::vector<String> irbrf_splitKeywords(const String &csv) {
    std::vector<String> out;
    String buf = csv;
    int idx;
    while ((idx = buf.indexOf(',')) >= 0) {
        String tok = buf.substring(0, idx);
        tok.trim();
        if (tok.length()) out.push_back(tok);
        buf = buf.substring(idx + 1);
    }
    buf.trim();
    if (buf.length()) out.push_back(buf);
    return out;
}


/*
 * irbrf_showExitMenu
 * On-screen replacement for the "m" (mark match) and "l" (loop file)
 * keyboard shortcuts, used on boards that have no physical keyboard
 * (for example the Cheap Yellow Display, which is touch only). Shown
 * when the user presses Esc on those boards instead of exiting right
 * away.
 *
 * Playback is expected to already be paused by the caller before this
 * is shown, since the menu blocks with loopOptions() the same way the
 * scan-cancel menu does elsewhere in this file.
 *
 * Returns true if the user chose to exit the bruteforce session,
 * false if they chose to close the menu and keep going. matches,
 * loopFile, and status are updated in place based on the choice made.
 */
static bool irbrf_showExitMenu(const String &curPath, std::vector<String> &matches,
                               bool &loopFile, String &status, int &sigIdx,
                               int sigTotal, std::vector<IrBrfSignal> &curSignals) {
    while (check(EscPress)) yield(); // drain the press that opened this menu

    bool alreadyMatched = false;
    for (const auto &m : matches)
        if (m == curPath) { alreadyMatched = true; break; }

    bool doExit = false;

    options = {
        {"Resume", [&]() {}},
        {"Prev Signal",
         [&]() {
             sigIdx = (sigIdx == 0) ? sigTotal - 1 : sigIdx - 1;
             status = "Prev signal";
         }},
        {"Next Signal",
         [&]() {
             sigIdx = (sigIdx == sigTotal - 1) ? 0 : sigIdx + 1;
             status = "Next signal";
         }},
        {"Send Signal",
         [&]() {
             const IrBrfSignal &manualSig = curSignals[sigIdx];
             status = "TX: " + manualSig.name;
             irbrf_sendSignal(manualSig);
         }},
        {alreadyMatched ? "Already Matched" : "Mark as Match",
         [&]() {
             if (!alreadyMatched) {
                 matches.push_back(curPath);
                 status = "Matched! (" + String(matches.size()) + " total)";
             }
         }},
        {loopFile ? "Loop File: ON" : "Loop File: OFF",
         [&]() {
             loopFile = !loopFile;
             status = loopFile ? "Loop: ON" : "Loop: OFF";
         }},
        {"Exit Bruteforce", [&]() { doExit = true; }},
    };
    loopOptions(options);
    options.clear();

    return doExit;
}

/*
 * ir_bruteforce
 * Entry point for the module, called from the menu. Overall flow:
 *   1. Check IR TX pin and pick a storage device and folder to scan.
 *   2. Ask the user whether to filter by keyword or use all .ir files,
 *      and how many cycles to send per file.
 *   3. Scan the folder tree for matching .ir files (irbrf_collectFiles),
 *      allowing the user to pause/resume or stop the scan and start
 *      with whatever was found so far.
 *   4. Loop through the matched files, sending each contained signal
 *      irbrf_interruptibleDelay(150) apart, for the configured number
 *      of cycles, while listening for Next/Prev/Pause/Match/Loop/Esc
 *      key presses to control playback.
 *   5. On exit, save any files the user marked as a match
 *      (irbrf_saveMatches) and show the finish screen.
 *
 * This function only runs when LITE_VERSION is not defined, since it
 * depends on SD card and filesystem access that the lite build does
 * not include.
 */

void ir_bruteforce() {
    checkIrTxPin();

    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No storage found", true);
        return;
    }

    String dirPath = pickDirectory(*fs);
    if (dirPath.length() == 0) return;
    if (dirPath.length() > 1 && dirPath.endsWith("/"))
        dirPath.remove(dirPath.length() - 1);

    bool allFiles = false;
    options = {
        {"Custom Btn Keyword", [&]() { allFiles = false; }},
        {"All IR files",       [&]() { allFiles = true; } },
    };
    loopOptions(options);
    options.clear();

    std::vector<String> keywords;
    if (!allFiles) {
        String kwInput = keyboard("power,on,off", 64, "Keywords (comma sep):");
        if (kwInput == "\x1B") return;
        keywords = irbrf_splitKeywords(kwInput);
    }

    String cycleStr = keyboard("3", 4, "Cycles per file:");
    if (cycleStr == "\x1B") return;
    int maxCycles = cycleStr.toInt();
    if (maxCycles < 1) maxCycles = 1;
    if (maxCycles > 50) maxCycles = 50;

    drawMainBorderWithTitle("IR Bruteforce");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("Scanning files...", tftWidth / 2, tftHeight / 2, 1);

    std::vector<IrBrfFile> files;
    std::vector<String> dirStack = {dirPath};
    int scannedFiles = 0;
    bool heapLimited = false;
    bool cancelled = false;

    while (true) {
        while (check(EscPress)) yield();
        irbrf_collectFiles(fs, dirStack, keywords, files, scannedFiles, heapLimited, cancelled);
        if (!cancelled) break;

        bool resume = false, startNow = false, giveUp = false;

        while (check(EscPress)) yield();
        options = {
            {"Continue Scanning",                                  [&]() { resume = true; }  },
            {"Start With Found (" + String(files.size()) + ")",    [&]() { startNow = true; }},
            {"Cancel",                                             [&]() { giveUp = true; }  },
        };
        loopOptions(options);
        options.clear();

        if (giveUp) return;
        if (startNow) break;
    }

    if (files.empty()) {
        displayError("No matching .ir files found", true);
        return;
    }

    char foundMsg[64];
    if (heapLimited) {
        snprintf(
            foundMsg, sizeof(foundMsg), "Found %d (stopped: low mem)", (int)files.size()
        );
        displayInfo(foundMsg, true);
    } else if ((int)files.size() >= IRBRF_MAX_FILES) {
        snprintf(
            foundMsg, sizeof(foundMsg), "Found %d+ files (capped). Enter to start...", (int)files.size()
        );
        displayInfo(foundMsg, true);
    } else {
        snprintf(foundMsg, sizeof(foundMsg), "Found %d files. Enter to start...", (int)files.size());
        displayInfo(foundMsg, true);
    }

    setup_ir_pin(bruceConfigPins.irTx, OUTPUT);


    int fileIdx  = 0;
    int fileTotal = (int)files.size();
    std::vector<String> matches;
    bool running = true;
    String status = "Running...";

    bool paused   = false;
    bool loopFile = false;

    std::vector<IrBrfSignal> curSignals;
    int loadedFileIdx = -1;

    while (running && fileIdx < fileTotal) {
        if (loadedFileIdx != fileIdx) {
            irbrf_loadFileSignals(fs, files[fileIdx].path, keywords, curSignals);
            loadedFileIdx = fileIdx;
        }
        const String &curPath = files[fileIdx].path;
        int sigTotal = (int)curSignals.size();
        int sigIdx   = 0;
        int cycle    = 1;

        if (sigTotal == 0) { fileIdx++; continue; }

        irbrf_drawStatic(curPath, fileIdx, fileTotal);
        irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                          (int)matches.size(), loopFile);

        while (running) {
            if (check(EscPress)) {
#ifdef HAS_KEYBOARD
                // Boards with a physical keyboard keep the fast path:
                // Esc saves matches and exits immediately, no confirm.
                running = false;
                break;
#else
                // Touch only boards (for example CYD) have no keys for
                // the "m" (match) and "l" (loop file) shortcuts below,
                // so Esc opens an on-screen menu offering the same
                // actions instead of exiting right away. Playback is
                // paused for as long as the menu is open.
                bool wasPaused = paused;
                paused = true;
                status = "Paused - Menu";
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);

                bool exitChosen = irbrf_showExitMenu(curPath, matches, loopFile, status,
                                                     sigIdx, sigTotal, curSignals);

                if (exitChosen) {
                    running = false;
                    break;
                }

                paused = wasPaused;
                // The menu drew over the whole screen, so the static
                // parts of the layout need to be redrawn before resuming.
                irbrf_drawStatic(curPath, fileIdx, fileTotal);
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);
                continue;
#endif
            }
            if (check(NextPress)) {          // > next file
                fileIdx = min(fileIdx + 1, fileTotal - 1);
                status = "Next file";
                break;
            }
            if (check(PrevPress)) {          // < prev file
                fileIdx = max(fileIdx - 1, 0);
                status = "Prev file";
                break;
            }
            if (check(SelPress)) {
                while (check(SelPress)) yield();
                paused = !paused;
                status = paused ? "Paused" : "Resumed";
            }

#ifdef HAS_KEYBOARD
            // "m" mark current file as a match, "l" toggle loop-current-file.
            // Only meaningful on boards with physical keys to press; on
            // touch only boards the same actions live in the Esc menu above.
            char pressedLetter = checkLetterShortcutPress();
            if (pressedLetter == 'm' || pressedLetter == 'M') {
                bool already = false;
                for (const auto &m : matches)
                    if (m == curPath) { already = true; break; }
                if (!already) {
                    matches.push_back(curPath);
                    status = "Matched! (" + String(matches.size()) + " total)";
                } else {
                    status = "Already marked";
                }
            }

            if (pressedLetter == 'l' || pressedLetter == 'L') {
                loopFile = !loopFile;
                status = loopFile ? "Loop: ON" : "Loop: OFF";
            }

            if (pressedLetter == 'o' || pressedLetter == 'O') {
                sigIdx = (sigIdx == 0) ? sigTotal - 1 : sigIdx - 1;
                status = "<: " + curSignals[sigIdx].name;
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);
            }

            if (pressedLetter == 'p' || pressedLetter == 'P') {
                sigIdx = (sigIdx == sigTotal - 1) ? 0 : sigIdx + 1;
                status = ">: " + curSignals[sigIdx].name;
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);
            }

            if (pressedLetter == 's' || pressedLetter == 'S') {
                const IrBrfSignal &manualSig = curSignals[sigIdx];
                status = "TX: " + manualSig.name;
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);
                irbrf_sendSignal(manualSig);
            }
#endif

            if (paused) {
                irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                                  (int)matches.size(), loopFile);
                delay(20);
                continue;
            }

            const IrBrfSignal &sig = curSignals[sigIdx];
            status = "TX: " + sig.name;
            irbrf_drawDynamic(sigIdx, sigTotal, cycle, maxCycles, paused, status,
                              (int)matches.size(), loopFile);

            irbrf_sendSignal(sig);
            irbrf_interruptibleDelay(150);

            sigIdx++;
            if (sigIdx >= sigTotal) {
                sigIdx = 0;
                cycle++;
                if (cycle > maxCycles) {
                    if (loopFile) {
                        cycle = 1;
                        status = "Looping file";
                    } else {
                        status = "Done, next file";
                        fileIdx++;
                        break;
                    }
                } else {
                    status = "Cycle " + String(cycle) + "/" + String(maxCycles);
                }
            }
        }
    }

    digitalWrite(bruceConfigPins.irTx, LED_OFF);

    String savedPath = irbrf_saveMatches(matches);
    irbrf_showFinishScreen(savedPath);

    returnToMenu = true;
}

#endif
