#include "universal_rf.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "modules/rf/rf_send.h"

#include <algorithm>
#include <functional>

#define UNIVERSAL_RF_ROOT "/UniversalRF"
#define UNIVERSAL_RF_ASSETS "/UniversalRF/assets"

// Upper bound for a single signal file the browser will buffer before sending.
// readSubFile itself caps the collected RAW bytes, so this only skips the
// multi-MB "chaos"/recording files that are useless from a browser.
#define MAX_SUB_FILE_BYTES (256 * 1024)

// Collect every .sub file path under `dir` (recursive), sorted.
static void collect_sub_files(FS &fs, const String &dir, std::vector<String> &files) {
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) return;
    std::vector<String> subdirs;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;
        if (isDir) {
            subdirs.push_back(fullPath);
        } else if (fullPath.endsWith(".sub") || fullPath.endsWith(".SUB")) {
            files.push_back(fullPath);
        }
    }
    root.close();
    std::sort(subdirs.begin(), subdirs.end());
    for (auto &d : subdirs) collect_sub_files(fs, d, files);
    std::sort(files.begin(), files.end());
}

static void list_subdirs(FS &fs, const String &dir, std::vector<String> &out) {
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) return;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;
        if (isDir) out.push_back(fullPath);
    }
    root.close();
    std::sort(out.begin(), out.end());
}

// Recursively search `dir` (up to `depth` levels) for a folder named `name`.
// Some zip extractors wrap the archive content in a folder named after the
// .zip (e.g. `BruceIR3.0-UniversalIR-RF-Full/UniversalIR/...`); this lets the
// DB be found even when it is nested inside such a wrapper. Returns the full
// path of the folder, or "" when not found. Hidden/system folders are skipped.
static String find_named_dir(FS &fs, const String &dir, const String &name, int depth) {
    File d = fs.open(dir);
    if (!d || !d.isDirectory()) return "";
    std::vector<String> subdirs;
    while (true) {
        bool isDir;
        String p = d.getNextFileName(&isDir);
        if (p == "") break;
        if (!isDir) continue;
        String n = p.substring(p.lastIndexOf("/") + 1);
        if (n.equalsIgnoreCase(name)) {
            d.close();
            return p;
        }
        subdirs.push_back(p);
    }
    d.close();
    if (depth <= 0) return "";
    for (auto &sd : subdirs) {
        String n = sd.substring(sd.lastIndexOf("/") + 1);
        if (n.startsWith(".")) continue;
        String r = find_named_dir(fs, sd, name, depth - 1);
        if (r != "") return r;
    }
    return "";
}

// DB root for the module. Uses /UniversalRF directly when present, otherwise
// scans for a folder with that name nested under the FS root (wrapper folder
// created by the extractor). Falls back to /UniversalRF when not found.
static String find_db_root(FS &fs, const String &name) {
    String direct = "/" + name;
    if (fs.exists(direct)) return direct;
    String found = find_named_dir(fs, "/", name, 4);
    return (found != "") ? found : direct;
}

// Send a .sub file. Returns true when the file carried a sendable signal.
// Reuses Bruce's standard read/transmit pipeline.
static bool send_sub_file(FS &fs, const String &path, int &status) {
    // Bruteforce-style RAW files (1-2 MB "chaos"/recording files in the Full
    // DB) can exhaust the heap when fully buffered by readSubFile. Skip them.
    File f = fs.open(path);
    if (!f) {
        status = 0;
        return false;
    }
    size_t sz = f.size();
    f.close();
    if (sz > MAX_SUB_FILE_BYTES) {
        status = -1;
        return false;
    }
    RfCodes data;
    if (!readSubFile(&fs, path, data)) return false;
    if (data.frequency == 0 || data.protocol.length() == 0) return false;
    return txSubFile(data, true);
}

// ---- Grid metrics / rendering (mirrors UniversalIR) ----------------------

struct GridMetrics {
    int cols, rows, cellW, cellH, gridX, gridY, pad, perPage;
};

static GridMetrics compute_grid_metrics() {
    GridMetrics m;
    const int HEADER = 46;
    int availW = tftWidth - 8;
    int availH = tftHeight - HEADER - 12;
    m.pad = 5;
    m.cols = (tftWidth > tftHeight) ? 4 : 2;
    int targetH = (tftWidth > tftHeight) ? 34 : 48;
    int rows = 2;
    int rowH = (availH - m.pad * (rows + 1)) / rows;
    while (rows < 6 && rowH >= targetH) {
        rows++;
        rowH = (availH - m.pad * (rows + 1)) / rows;
    }
    while (rows > 2 && rowH < 26) {
        rows--;
        rowH = (availH - m.pad * (rows + 1)) / rows;
    }
    m.rows = rows;
    m.cellH = rowH;
    m.cellW = (availW - m.pad * (m.cols + 1)) / m.cols;
    m.gridX = (tftWidth - (m.cols * m.cellW + (m.cols + 1) * m.pad)) / 2;
    m.gridY = HEADER;
    m.perPage = m.cols * m.rows;
    return m;
}

static void render_page(
    const GridMetrics &m, const std::vector<String> &labels, int total, int page, int sel
) {
    int pages = (total + m.perPage - 1) / m.perPage;
    int areaW = m.cols * m.cellW + (m.cols + 1) * m.pad;
    int areaH = m.rows * m.cellH + (m.rows + 1) * m.pad;
    tft.fillRect(m.gridX, m.gridY, areaW, areaH, bruceConfig.bgColor);

    for (int i = 0; i < m.perPage; i++) {
        int bi = page * m.perPage + i;
        if (bi >= total) continue;
        int col = i % m.cols;
        int row = i / m.cols;
        int x = m.gridX + m.pad + col * (m.cellW + m.pad);
        int y = m.gridY + m.pad + row * (m.cellH + m.pad);
        bool selc = (bi == sel);
        uint16_t fg = selc ? bruceConfig.bgColor : bruceConfig.priColor;
        uint16_t bg = selc ? bruceConfig.priColor : bruceConfig.bgColor;
        uint16_t border = selc ? TFT_WHITE : bruceConfig.secColor;
        tft.fillRoundRect(x, y, m.cellW, m.cellH, 3, bg);
        tft.drawRoundRect(x, y, m.cellW, m.cellH, 3, border);
        String label = labels[bi];
        tft.setTextColor(fg, bg);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(FP);
        tft.setTextSize(1);
        while (label.length() > 0 && tft.textWidth(label) > m.cellW - 8) label.remove(label.length() - 1);
        if (label.length() != labels[bi].length()) label += "~";
        tft.drawString(label, x + m.cellW / 2, y + m.cellH / 2);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextFont(FP);
    tft.setTextSize(1);
    tft.setTextDatum(BR_DATUM);
    String pg = String(page + 1) + "/" + String(pages);
    tft.drawString(pg, tftWidth - 8, tftHeight - 4);
    String seld = labels[sel];
    while (seld.length() > 0 && tft.textWidth(seld) > tftWidth / 2 - 8) seld.remove(seld.length() - 1);
    if (seld.length() != labels[sel].length()) seld += "~";
    tft.setTextDatum(BL_DATUM);
    tft.drawString(seld, 8, tftHeight - 4);
}

// Human-readable label from a file/folder name: strip the .sub extension, drop
// recording prefixes, turn separators into spaces.
static String pretty_name(String s) {
    if (s.endsWith(".sub")) s.remove(s.length() - 4);
    else if (s.endsWith(".SUB")) s.remove(s.length() - 4);
    if (s.startsWith("RAW-RECORDING")) s = "Recording " + s.substring(13);
    else if (s.startsWith("RAW_RECORDING")) s = "Recording " + s.substring(13);
    else if (s.startsWith("RAW-")) s.remove(0, 4);
    else if (s.startsWith("RAW_")) s.remove(0, 4);
    s.replace('_', ' ');
    s.replace('-', ' ');
    while (s.indexOf("  ") >= 0) s.replace("  ", " ");
    s.trim();
    if (s.length() == 0) s = "?";
    return s;
}

static std::vector<String> labels_for(const std::vector<String> &files) {
    std::vector<String> labels;
    for (auto &fp : files) {
        String name = fp.substring(fp.lastIndexOf("/") + 1);
        labels.push_back(pretty_name(name));
    }
    return labels;
}

// Render + navigate a paged grid of labels. `on_select(sel)` runs on a long SEL
// press; return true from it to exit the grid (ESC also exits).
static void grid_navigate(
    const std::vector<String> &labels, const String &title,
    const std::function<bool(int)> &on_select
) {
    if (labels.empty()) return;
    int total = labels.size();
    GridMetrics m = compute_grid_metrics();
    int sel = 0;
    int page = 0;
    unsigned long openTs = millis();

    drawMainBorderWithTitle(title);
    render_page(m, labels, total, page, sel);

    while (true) {
#ifdef HAS_3_BUTTONS
        if (EscPress && PrevPress) EscPress = false;
#endif
        if (check(EscPress)) break;

        bool moved = false;
#ifdef HAS_ENCODER
        int32_t rot = drainRotarySteps();
        if (rot != 0) {
            check(PrevPress);
            check(NextPress);
            check(UpPress);
            check(DownPress);
            int dir = (rot > 0) ? -1 : 1;
            int steps = (rot > 0) ? (int)rot : (int)-rot;
            for (int i = 0; i < steps; i++) sel = (sel + dir + total) % total;
            moved = true;
            vTaskDelay(4 / portTICK_PERIOD_MS);
        } else
#endif
        {
            if (check(PrevPress)) {
                sel = (sel - 1 + total) % total;
                moved = true;
            }
            if (check(NextPress)) {
                sel = (sel + 1) % total;
                moved = true;
            }
            if (check(UpPress)) {
                sel = (sel - m.cols + total) % total;
                moved = true;
            }
            if (check(DownPress)) {
                sel = (sel + m.cols) % total;
                moved = true;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        int newPage = sel / m.perPage;
        if (newPage != page) {
            page = newPage;
            moved = true;
        }
        if (moved) render_page(m, labels, total, page, sel);

        if (millis() - openTs > 600 && check(SelPress)) {
            if (on_select(sel)) break;
            drawMainBorderWithTitle(title);
            render_page(m, labels, total, page, sel);
            openTs = millis();
        }
    }
}

// Grid screen for the .sub files inside one device folder.
static void sub_grid(FS &fs, const std::vector<String> &files, const String &title) {
    if (files.empty()) {
        displayError("No .sub files found");
        delay(1200);
        return;
    }

    std::vector<String> labels = labels_for(files);
    labels.push_back("Back");
    grid_navigate(labels, title, [&fs, &labels, &files](int sel) {
        if (sel >= (int)files.size()) return true;
        int status = 0;
        if (send_sub_file(fs, files[sel], status)) {
            displaySuccess("Sent " + labels[sel]);
            delay(600);
        } else if (status == -1) {
            displayError("File too big to send");
            delay(900);
        } else {
            displayError("Failed to send");
            delay(600);
        }
        return false;
    });
}

// ---- Built-in generic test signals ---------------------------------------
//
// A "Generic" category that always exists, independent of the DB files on the
// storage, so any board with a working RF module can be tested. Signals go
// through the standard sendRfCommand pipeline (same init/deinit path as the DB
// files).

static void generic_send_raw(uint32_t freq_hz, const std::vector<int> &timings) {
    RfCodes c;
    c.frequency = freq_hz;
    c.protocol = "RAW";
    c.preset = "Ook650Async";
    String data;
    for (size_t i = 0; i < timings.size(); i++) {
        if (i) data += " ";
        data += String(timings[i]);
    }
    c.data = data;
    sendRfCommand(c, true);
}

static void generic_send_keyfob(uint32_t freq_hz, uint64_t key, int bits, int te) {
    RfCodes c;
    c.frequency = freq_hz;
    c.protocol = "Princeton";
    c.preset = "Ook650Async";
    c.key = key;
    c.Bit = bits;
    c.te = te;
    sendRfCommand(c, true);
}

// A ~0.5 s square-wave burst on the given band: checks the TX module and lets a
// receiver / keyfob verify the frequency.
static std::vector<int> carrier_burst(int half_us, int cycles) {
    std::vector<int> t;
    t.reserve((size_t)cycles * 2);
    for (int i = 0; i < cycles; i++) {
        t.push_back(half_us);
        t.push_back(-half_us);
    }
    return t;
}

static void generic_menu() {
    std::vector<String> labels = {
        "Carrier 433 MHz",
        "Carrier 315 MHz",
        "Carrier 868 MHz",
        "OOK Keyfob 433 MHz",
        "OOK Keyfob 315 MHz",
        "Doorbell 433 MHz",
        "Back",
    };
    grid_navigate(labels, "Generic", [](int sel) {
        const char *what = nullptr;
        switch (sel) {
            case 0: generic_send_raw(433920000, carrier_burst(350, 700)); what = "Carrier 433 MHz"; break;
            case 1: generic_send_raw(315000000, carrier_burst(350, 700)); what = "Carrier 315 MHz"; break;
            case 2: generic_send_raw(868000000, carrier_burst(350, 700)); what = "Carrier 868 MHz"; break;
            case 3: generic_send_keyfob(433920000, 0x5A5A5A, 24, 333); what = "OOK Keyfob 433 MHz"; break;
            case 4: generic_send_keyfob(315000000, 0x5A5A5A, 24, 333); what = "OOK Keyfob 315 MHz"; break;
            case 5:
                generic_send_raw(433920000, {900, -300, 900, -300, 900, -300, 900, -1200,
                                             900, -300, 900, -300, 900, -300, 900, -300});
                what = "Doorbell 433 MHz";
                break;
            default: return true;
        }
        displaySuccess("Sent " + String(what));
        delay(600);
        return false;
    });
}

// Browse the devices (subfolders) of a category. Subfolders that contain
// .sub files directly are listed as grid entries; any deeper nesting is
// flattened by collecting .sub files recursively.
//
// cat_path/title are taken BY VALUE: they alias captured String members of the
// caller's std::function closure (which lives in the shared global `options`
// vector). This function calls options.clear() (see below), which destroys that
// closure, so any reference into it would dangle. Owning a copy here survives
// the clear.
static void browse_devices(FS &fs, String cat_path, String title) {
    std::vector<String> subdirs;
    list_subdirs(fs, cat_path, subdirs);

    // Collect only direct .sub files at the category level (flat categories).
    std::vector<String> direct;
    {
        File root = fs.open(cat_path);
        if (root && root.isDirectory()) {
            while (true) {
                bool isDir;
                String fullPath = root.getNextFileName(&isDir);
                if (fullPath == "") break;
                if (!isDir && (fullPath.endsWith(".sub") || fullPath.endsWith(".SUB"))) {
                    direct.push_back(fullPath);
                }
            }
        }
        root.close();
        std::sort(direct.begin(), direct.end());
    }

    bool exit_flow = false;
    while (!exit_flow) {
        options.clear();
        if (!subdirs.empty()) {
            // Mirrors the UniversalIR "Brands" flow: one entry that opens a
            // grid of the brand subfolders, each of which leads to its signals.
            options.push_back({"Brands (" + String(subdirs.size()) + ")", [&fs, subdirs, title]() {
                std::vector<String> paths;
                std::vector<String> labels;
                for (auto &d : subdirs) {
                    paths.push_back(d);
                    labels.push_back(pretty_name(d.substring(d.lastIndexOf("/") + 1)));
                }
                labels.push_back("Back");
                grid_navigate(labels, title, [&fs, &paths](int sel) {
                    if (sel >= (int)paths.size()) return true;
                    std::vector<String> files;
                    collect_sub_files(fs, paths[sel], files);
                    if (files.empty()) {
                        displayError("No .sub files found");
                        delay(1200);
                        return false;
                    }
                    sub_grid(fs, files, pretty_name(paths[sel].substring(paths[sel].lastIndexOf("/") + 1)));
                    return false;
                });
            }});
        }
        if (!direct.empty()) {
            options.push_back({"Direct signals (" + String(direct.size()) + ")", [&fs, direct, title]() {
                sub_grid(fs, direct, title);
            }});
        }
        options.push_back({"Back", [&]() { exit_flow = true; }});
        int r = loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
        if (r < 0) break;
    }
    options.clear();
}

static void show_categories() {
    FS *fsPtr = nullptr;
#if defined(UNIVERSAL_RF_LITTLEFS_ONLY)
    if (setupLittleFS()) {
        fsPtr = &LittleFS;
    }
#else
    if (setupSdCard()) {
        fsPtr = &SD;
    } else if (setupLittleFS()) {
        fsPtr = &LittleFS;
    }
#endif
    if (fsPtr == nullptr) {
        displayError("No storage found");
        delay(1500);
        return;
    }

    FS &fs = *fsPtr;

    String dbRoot = find_db_root(fs, "UniversalRF");
    String assetsRoot = dbRoot + "/assets";

    if (!fs.exists(dbRoot)) {
        fs.mkdir(dbRoot);
#if defined(UNIVERSAL_RF_LITTLEFS_ONLY)
        displayError("Put RF files in /UniversalRF (LittleFS)");
#else
        displayError("Put RF files in /UniversalRF (SD or LittleFS)");
#endif
        delay(2000);
        return;
    }

    std::vector<String> cats;
    String roots[2] = {assetsRoot, dbRoot};
    for (int r = 0; r < 2; r++) {
        File root = fs.open(roots[r]);
        if (!root || !root.isDirectory()) continue;
        while (true) {
            bool isDir;
            String fullPath = root.getNextFileName(&isDir);
            if (fullPath == "") break;
            if (isDir) {
                String name = fullPath.substring(fullPath.lastIndexOf("/") + 1);
                bool dup = false;
                for (auto &c : cats) {
                    if (c.equalsIgnoreCase(name)) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) cats.push_back(fullPath);
            }
        }
        root.close();
    }

    if (cats.empty()) {
        displayError("No category folders found");
        delay(2000);
        return;
    }

    returnToMenu = false;
    while (!returnToMenu) {
        options.clear();
        // Built-in test category: always present as the first option, no DB
        // files required. A DB folder with the same name is skipped below.
        options.push_back({"Generic", [&]() { generic_menu(); }});
        for (auto &cat : cats) {
            String name = cat.substring(cat.lastIndexOf("/") + 1);
            if (name.equalsIgnoreCase("Generic")) continue;
            options.push_back({pretty_name(name).c_str(), [&fs, cat, name]() {
                browse_devices(fs, cat, pretty_name(name));
            }});
        }
        options.push_back({"Main Menu", [&]() { returnToMenu = true; }});
        loopOptions(options);
    }
    options.clear();
}

void universalRFcodes() {
    show_categories();
}
