#include "universal_ir.h"
#include "TV-B-Gone.h"
#include "custom_ir.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"

#include <map>

#define UNIVERSAL_IR_ROOT "/UniversalIR"
#define UNIVERSAL_IR_ASSETS "/UniversalIR/assets"
#define ORIENT_FILE "/UniversalIR/orient.txt"

// Resolved DB root for the running FS (see find_db_root). Some zip extractors
// wrap the archive content in a folder named after the .zip (e.g.
// `BruceIR3.0-UniversalIR-RF-Full/UniversalIR/...`); when /UniversalIR is not
// at the FS root we search for it and keep the found path here.
static String g_ir_root = UNIVERSAL_IR_ROOT;

// Recursively search `dir` (up to `depth` levels) for a folder named `name`.
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

// DB root for the module. Uses /UniversalIR directly when present, otherwise
// scans for a nested folder with that name. Falls back to /UniversalIR.
static String find_db_root(FS &fs, const String &name) {
    String direct = "/" + name;
    if (fs.exists(direct)) return direct;
    String found = find_named_dir(fs, "/", name, 4);
    return (found != "") ? found : direct;
}

enum RemoteOrient { ORIENT_AUTO = 0, ORIENT_GRID, ORIENT_LIST };

static void apply_display_orientation(int r) {
    if (r < 0 || r > 3) return;
    bruceConfigPins.rotation = r;
    tft.setRotation(r);
    tft.setRotation(r);
    if (r & 0b01) {
        tftWidth = TFT_HEIGHT;
#if defined(HAS_TOUCH)
        tftHeight = TFT_WIDTH - 20;
#else
        tftHeight = TFT_WIDTH;
#endif
    } else {
        tftWidth = TFT_WIDTH;
#if defined(HAS_TOUCH)
        tftHeight = TFT_HEIGHT - 20;
#else
        tftHeight = TFT_HEIGHT;
#endif
    }
}

static int load_ir_orient(FS &fs, int fallback) {
    File f = fs.open(g_ir_root + "/orient.txt", FILE_READ);
    if (!f) return fallback;
    int v = f.readStringUntil('\n').toInt();
    f.close();
    if (v < 0 || v > 3) return fallback;
    return v;
}

static void save_ir_orient(FS &fs, int v) {
    if (v < 0 || v > 3) return;
    File f = fs.open(g_ir_root + "/orient.txt", FILE_WRITE);
    if (!f) return;
    f.println(v);
    f.close();
}

struct ButtonDef {
    String label;
    std::vector<String> search;
};

struct CategoryConfig {
    String flat_file;
    int orientation = ORIENT_AUTO;
    std::vector<ButtonDef> buttons;
};

static std::vector<ButtonDef> default_tv_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle"}});
    b.push_back({"Mute", {"Mute"}});
    b.push_back({"Vol +", {"Vol_up", "Vol+", "Vol_inc", "Volume_up"}});
    b.push_back({"Vol -", {"Vol_dn", "Vol-", "Vol_dec", "Volume_dn"}});
    b.push_back({"Ch +", {"Ch_next", "Ch+", "Ch_inc", "Channel_up"}});
    b.push_back({"Ch -", {"Ch_prev", "Ch-", "Ch_dec", "Channel_dn"}});
    b.push_back({"Input", {"Input", "Source", "Input_source", "AV"}});
    b.push_back({"Menu", {"Menu"}});
    b.push_back({"Exit", {"Exit", "Exit_menu"}});
    b.push_back({"OK", {"OK", "Select", "Enter"}});
    b.push_back({"Up", {"Up", "Up_arrow", "Arrow_up"}});
    b.push_back({"Down", {"Down", "Down_arrow", "Arrow_down"}});
    b.push_back({"Left", {"Left", "Left_arrow", "Arrow_left"}});
    b.push_back({"Right", {"Right", "Right_arrow", "Arrow_right"}});
    b.push_back({"Info", {"Info"}});
    b.push_back({"0", {"0"}});
    b.push_back({"1", {"1"}});
    b.push_back({"2", {"2"}});
    b.push_back({"3", {"3"}});
    b.push_back({"4", {"4"}});
    b.push_back({"5", {"5"}});
    b.push_back({"6", {"6"}});
    b.push_back({"7", {"7"}});
    b.push_back({"8", {"8"}});
    b.push_back({"9", {"9"}});
    return b;
}

static std::vector<ButtonDef> default_ac_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle", "Power_on", "Off"}});
    b.push_back({"Cool", {"Cool", "Cool_hi", "Cool_lo", "Cool_high", "Cool_low"}});
    b.push_back({"Heat", {"Heat", "Heat_hi", "Heat_lo", "Heat_high", "Heat_low"}});
    b.push_back({"Dry", {"Dry", "Dh", "Dry_mode", "Dehumidify"}});
    b.push_back({"Mode", {"Mode", "Mode_switch"}});
    b.push_back({"Fan", {"Fan", "Fan_speed", "Fan_auto", "Fan_1", "Fan_2", "Fan_3"}});
    b.push_back({"Temp +", {"Temp_up", "Temp+", "Temperature_up"}});
    b.push_back({"Temp -", {"Temp_dn", "Temp-", "Temperature_down"}});
    b.push_back({"Swing", {"Swing", "Swing_switch"}});
    b.push_back({"Sleep", {"Sleep", "Sleep_mode"}});
    b.push_back({"Timer", {"Timer"}});
    return b;
}

static std::vector<ButtonDef> default_audio_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle"}});
    b.push_back({"Vol +", {"Vol_up", "Vol+", "Volume_up"}});
    b.push_back({"Vol -", {"Vol_dn", "Vol-", "Volume_dn"}});
    b.push_back({"Mute", {"Mute"}});
    b.push_back({"Play", {"Play", "Play_pause"}});
    b.push_back({"Pause", {"Pause"}});
    b.push_back({"Next", {"Next", "Next_track", "Skip_next"}});
    b.push_back({"Prev", {"Prev", "Prev_track", "Skip_prev"}});
    return b;
}

static std::vector<ButtonDef> default_fan_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle"}});
    b.push_back({"Speed +", {"Speed_up", "Speed+", "Speed_inc"}});
    b.push_back({"Speed -", {"Speed_dn", "Speed-", "Speed_dec"}});
    b.push_back({"Mode", {"Mode"}});
    b.push_back({"Timer", {"Timer"}});
    b.push_back({"Rotate", {"Rotate", "Swing", "Oscillate"}});
    return b;
}

static std::vector<ButtonDef> default_led_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle", "Power_on", "Power_off"}});
    b.push_back({"Bright +", {"Brightness_up", "Bright+", "Brightness+"}});
    b.push_back({"Bright -", {"Brightness_dn", "Bright-", "Brightness-"}});
    b.push_back({"Red", {"Red"}});
    b.push_back({"Green", {"Green"}});
    b.push_back({"Blue", {"Blue"}});
    b.push_back({"White", {"White"}});
    b.push_back({"Mode", {"Mode"}});
    return b;
}

static std::vector<ButtonDef> default_projector_layout() {
    std::vector<ButtonDef> b;
    b.push_back({"Power", {"Power", "Power_toggle"}});
    b.push_back({"Vol +", {"Vol_up", "Vol+", "Volume_up"}});
    b.push_back({"Vol -", {"Vol_dn", "Vol-", "Volume_dn"}});
    b.push_back({"Mute", {"Mute"}});
    b.push_back({"Menu", {"Menu"}});
    b.push_back({"Input", {"Input", "Source"}});
    b.push_back({"OK", {"OK", "Select", "Enter"}});
    b.push_back({"Up", {"Up"}});
    b.push_back({"Down", {"Down"}});
    b.push_back({"Left", {"Left"}});
    b.push_back({"Right", {"Right"}});
    b.push_back({"Freeze", {"Freeze"}});
    return b;
}

static uint32_t fnv1a(const String &s) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        h ^= (uint8_t)c;
        h *= 16777619u;
    }
    return h;
}

static bool matches_hash(uint32_t h, const std::vector<String> &search) {
    for (auto &s : search) {
        if (fnv1a(s) == h) return true;
    }
    return false;
}

struct SigEntry {
    uint32_t offset;
    uint32_t hash;
};

struct FileSig {
    String path;
    std::vector<SigEntry> sigs;
};

typedef std::vector<FileSig> SigIndex;

static FileSig build_file_index(FS &fs, const String &path) {
    FileSig file;
    file.path = path;
    File f = fs.open(path, FILE_READ);
    if (!f) return file;
    while (f.available()) {
        uint32_t pos = (uint32_t)f.position();
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        if (line.startsWith("name:")) {
            String name = line.substring(5);
            name.trim();
            if (name.length() > 0) file.sigs.push_back({pos, fnv1a(name)});
        }
    }
    f.close();
    return file;
}

static void collect_ir_files(FS &fs, const String &dir, std::vector<String> &files) {
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) return;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;
        if (isDir) {
            collect_ir_files(fs, fullPath, files);
        } else if (fullPath.endsWith(".ir")) {
            files.push_back(fullPath);
        }
    }
    root.close();
}

static SigIndex build_dir_index(FS &fs, const String &dir) {
    SigIndex idx;
    std::vector<String> files;
    collect_ir_files(fs, dir, files);
    for (auto &fp : files) {
        FileSig file = build_file_index(fs, fp);
        if (!file.sigs.empty()) idx.push_back(file);
    }
    return idx;
}

static SigIndex build_flat_index(FS &fs, const String &path) {
    SigIndex idx;
    FileSig file = build_file_index(fs, path);
    if (!file.sigs.empty()) idx.push_back(file);
    return idx;
}

static std::vector<String> names_in_file(FS &fs, const String &path) {
    std::vector<String> names;
    File f = fs.open(path, FILE_READ);
    if (!f) return names;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        if (line.startsWith("name:")) {
            String n = line.substring(5);
            n.trim();
            if (n.length() > 0) names.push_back(n);
        }
    }
    f.close();
    return names;
}

static bool read_signal_open(File &f, IRCode &code) {
    bool in_sig = false;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        if (line.startsWith("name:")) {
            if (in_sig) break;
            code.name = line.substring(5);
            code.name.trim();
            in_sig = true;
        } else if (in_sig) {
            if (line.length() == 0 || line.startsWith("#")) break;
            if (line.startsWith("type:")) {
                code.type = line.substring(5);
                code.type.trim();
            } else if (line.startsWith("protocol:")) {
                code.protocol = line.substring(9);
                code.protocol.trim();
            } else if (line.startsWith("address:")) {
                code.address = line.substring(8);
                code.address.trim();
            } else if (line.startsWith("command:")) {
                code.command = line.substring(8);
                code.command.trim();
            } else if (line.startsWith("frequency:")) {
                code.frequency = (uint16_t)line.substring(10).toInt();
            } else if (line.startsWith("bits:")) {
                code.bits = (uint8_t)line.substring(5).toInt();
            } else if (line.startsWith("data:") || line.startsWith("value:") || line.startsWith("state:")) {
                code.data = line.substring(line.indexOf(':') + 1);
                code.data.trim();
            }
        }
    }
    return in_sig;
}

static bool sendable(const IRCode &c) {
    if (c.type.equalsIgnoreCase("raw")) return (c.frequency != 0 && c.data.length() > 0);
    return c.protocol.length() > 0;
}

static void send_progress_ui(int sent, int total, const String &brand, const String &signal_name, bool start) {
    if (start) {
        tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
        tft.drawRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.priColor);
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setCursor((tftWidth - signal_name.length() * LW * FM) / 2, tftHeight / 2 - 34);
    tft.print(signal_name);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor((tftWidth - brand.length() * LW * FM) / 2, tftHeight / 2 - 16);
    tft.print(brand);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    String cnt = String(sent) + "/" + String(total);
    tft.setCursor((tftWidth - cnt.length() * LW * FP) / 2, tftHeight / 2 + 6);
    tft.print(cnt);

    int barWidth = map(sent, 0, total, 0, tftWidth - 40);
    if (barWidth < 3) barWidth = 3;
    tft.fillRect(20, tftHeight - 45, barWidth, 13, bruceConfig.priColor);
}

static int spam_index(
    FS &fs, const SigIndex &idx, const std::vector<String> &search, const String &brand, const String &display_name
) {
    checkIrTxPin();

    int total = 0;
    for (auto &file : idx) {
        for (auto &sig : file.sigs) {
            if (matches_hash(sig.hash, search)) total++;
        }
    }
    if (total == 0) return 0;

    int sent = 0;
    for (auto &file : idx) {
        File f = fs.open(file.path, FILE_READ);
        if (!f) continue;
        for (auto &sig : file.sigs) {
            if (!matches_hash(sig.hash, search)) continue;
            IRCode code;
            if (!f.seek(sig.offset)) continue;
            if (!read_signal_open(f, code)) continue;
            if (!sendable(code)) continue;
            sendIRCommand(&code, true);
            sent++;
            send_progress_ui(sent, total, brand, display_name, sent == 1);
            if (check(SelPress)) { // Pause spam (SEL again resumes, ESC aborts)
                while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
                tft.fillRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.bgColor);
                tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                tft.setTextDatum(MC_DATUM);
                tft.setTextFont(FP);
                tft.setTextSize(1);
                tft.drawString("PAUSED - SEL to resume", tftWidth / 2, tftHeight - 38);
                while (!check(SelPress)) {
                    if (check(EscPress)) {
                        f.close();
                        return -1;
                    }
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                while (check(SelPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
                tft.drawRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.priColor);
                send_progress_ui(sent, total, brand, display_name, false);
            }
            if (check(EscPress)) {
                f.close();
                return -1;
            }
        }
        f.close();
    }
    return sent;
}

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

static void render_page(const GridMetrics &m, const std::vector<ButtonDef> &btns, int total, int page, int sel) {
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
        String label = btns[bi].label;
        tft.setTextColor(fg, bg);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(FP);
        tft.setTextSize(1);
        while (label.length() > 0 && tft.textWidth(label) > m.cellW - 8) label.remove(label.length() - 1);
        if (label.length() != btns[bi].label.length()) label += "~";
        tft.drawString(label, x + m.cellW / 2, y + m.cellH / 2);
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextFont(FP);
    tft.setTextSize(1);
    tft.setTextDatum(BR_DATUM);
    String pg = String(page + 1) + "/" + String(pages);
    tft.drawString(pg, tftWidth - 8, tftHeight - 4);
    String seld = btns[sel].label;
    while (seld.length() > 0 && tft.textWidth(seld) > tftWidth / 2 - 8) seld.remove(seld.length() - 1);
    if (seld.length() != btns[sel].label.length()) seld += "~";
    tft.setTextDatum(BL_DATUM);
    tft.drawString(seld, 8, tftHeight - 4);
}

static void show_brands_flow(
    FS &fs, String cat_path, const std::vector<ButtonDef> &buttons, int orientation, String title
);

static bool remote_grid(
    FS &fs, const SigIndex &idx, const std::vector<ButtonDef> &buttons, const String &title,
    const String &spam_brand, const String &brands_path, int orientation
) {
    std::vector<ButtonDef> btns = buttons;
    if (brands_path.length() > 0) btns.push_back({"Brands", {}});
    btns.push_back({"Orient", {}});
    btns.push_back({"Back", {}});

    int total = btns.size();
    GridMetrics m = compute_grid_metrics();
    int sel = 0;
    int page = 0;
    unsigned long openTs = millis();

    drawMainBorderWithTitle(title);
    render_page(m, btns, total, page, sel);

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
        if (moved) render_page(m, btns, total, page, sel);

        if (millis() - openTs > 600 && check(SelPress)) {
            ButtonDef &btn = btns[sel];
            if (btn.label == "Back") break;
            if (btn.label == "Orient") {
                gsetRotation(true);
                returnToMenu = false;
                return true;
            }
            if (btn.label == "Brands") {
                show_brands_flow(fs, brands_path, buttons, orientation, title);
                drawMainBorderWithTitle(title);
                render_page(m, btns, total, page, sel);
                openTs = millis();
                continue;
            }
            int sent = spam_index(fs, idx, btn.search, spam_brand, btn.label);
            if (sent < 0) {
                displayWarning("Stopped");
                delay(1000);
            } else if (sent > 0) {
                displaySuccess(String(sent) + " sent");
                delay(1000);
            } else {
                displayError("No signals sent");
                delay(1000);
            }
            drawMainBorderWithTitle(title);
            render_page(m, btns, total, page, sel);
            openTs = millis();
        }
    }
    return false;
}

static void show_remote(
    FS &fs, const SigIndex &idx, const std::vector<ButtonDef> &buttons, String title, String spam_brand,
    String brands_path, int orientation
) {
    (void)orientation;
    while (true) {
        bool rotated = remote_grid(fs, idx, buttons, title, spam_brand, brands_path, ORIENT_GRID);
        if (!rotated) break;
    }
}

static void generic_signal_list(
    FS &fs, const SigIndex &idx, String brand, String title, String brands_path, int orientation
) {
    std::map<String, int> name_counts;
    for (auto &file : idx) {
        std::vector<String> names = names_in_file(fs, file.path);
        for (auto &n : names) name_counts[n]++;
    }
    if (name_counts.empty()) {
        displayError("No signals found");
        delay(1500);
        return;
    }

    bool exit_list = false;
    while (!exit_list) {
        options.clear();
        int added = 0;
        for (auto &entry : name_counts) {
            if (added >= 150) break;
            added++;
            String label = entry.first;
            if (entry.second > 1) label += " (" + String(entry.second) + ")";
            String sig_name = entry.first;
            options.push_back({label.c_str(), [&fs, &idx, brand, sig_name]() {
                std::vector<String> search;
                search.push_back(sig_name);
                int sent = spam_index(fs, idx, search, brand, sig_name);
                if (sent < 0) {
                    displayWarning("Stopped");
                    delay(1000);
                } else if (sent > 0) {
                    displaySuccess(String(sent) + " sent");
                    delay(1000);
                } else {
                    displayError("No signals sent");
                    delay(1000);
                }
            }});
        }
        if (brands_path.length() > 0) {
            String bp = brands_path;
            options.push_back({"Brands", [&fs, bp, orientation, title]() {
                std::vector<ButtonDef> empty;
                show_brands_flow(fs, bp, empty, orientation, title);
            }});
        }
        options.push_back({"Back", [&]() { exit_list = true; }});
        int r = loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
        if (r < 0) break;
    }
    options.clear();
}

// NOTE: cat_path/title are BY VALUE (not const&). They alias captured String
// members of the caller's closure in the shared global `options` vector; the
// options.clear() below destroys that closure, so a reference would dangle.
static void show_brands_flow(
    FS &fs, String cat_path, const std::vector<ButtonDef> &buttons, int orientation, String title
) {
    std::vector<String> brands;
    File root = fs.open(cat_path);
    if (!root || !root.isDirectory()) return;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;
        if (isDir) brands.push_back(fullPath);
    }
    root.close();

    if (brands.empty()) {
        displayError("No brands found");
        delay(1500);
        return;
    }

    bool exit_flow = false;
    while (!exit_flow) {
        options.clear();
        for (auto &brand_path : brands) {
            String brand = brand_path.substring(brand_path.lastIndexOf("/") + 1);
            options.push_back({brand.c_str(), [&fs, &buttons, brand_path, brand, orientation, title]() {
                SigIndex idx = build_dir_index(fs, brand_path);
                if (idx.empty()) {
                    displayError("No .ir files in " + brand);
                    delay(1500);
                    return;
                }
                if (buttons.empty()) {
                    generic_signal_list(fs, idx, brand, brand, "", orientation);
                } else {
                    show_remote(fs, idx, buttons, brand, brand, "", orientation);
                }
            }});
        }
        options.push_back({"Back", [&]() { exit_flow = true; }});
        int r = loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
        if (r < 0) break;
    }
    options.clear();
}

static std::vector<ButtonDef> layout_for(const String &lower) {
    if (lower.startsWith("tv")) return default_tv_layout();
    if (lower.startsWith("ac") || lower.startsWith("air")) return default_ac_layout();
    if (lower.startsWith("audio")) return default_audio_layout();
    if (lower.startsWith("fan")) return default_fan_layout();
    if (lower.startsWith("led")) return default_led_layout();
    if (lower.startsWith("proj")) return default_projector_layout();
    return {};
}

static String builtin_flat_for(const String &lower) {
    if (lower.startsWith("tv")) return "tv.ir";
    if (lower.startsWith("ac") || lower.startsWith("air")) return "ac.ir";
    if (lower.startsWith("audio")) return "audio.ir";
    if (lower.startsWith("fan")) return "fans.ir";
    if (lower.startsWith("led")) return "leds.ir";
    if (lower.startsWith("proj")) return "projectors.ir";
    return "";
}

static String resolve_flat(FS &fs, const String &name) {
    if (name.length() == 0) return "";
    if (name.startsWith("/")) return fs.exists(name) ? name : "";
    String a = g_ir_root + "/assets/" + name;
    if (fs.exists(a)) return a;
    String r = g_ir_root + "/" + name;
    if (fs.exists(r)) return r;
    return "";
}

static bool load_layouts(FS &fs, std::map<String, CategoryConfig> &configs) {
    File f = fs.open(g_ir_root + "/layouts.ini", FILE_READ);
    if (!f) return false;

    String section;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            section = line.substring(1, line.length() - 1);
            section.toLowerCase();
            configs[section];
        } else if (section.length() > 0) {
            int colon = line.indexOf(':');
            if (colon <= 0) continue;
            String key = line.substring(0, colon);
            key.trim();
            String val = line.substring(colon + 1);
            val.trim();
            if (key.length() == 0 || val.length() == 0) continue;

            CategoryConfig &cfg = configs[section];
            String lkey = key;
            lkey.toLowerCase();

            if (lkey == "orientation") {
                String v = val;
                v.toLowerCase();
                if (v == "grid") cfg.orientation = ORIENT_GRID;
                else if (v == "list") cfg.orientation = ORIENT_LIST;
                else cfg.orientation = ORIENT_AUTO;
            } else if (lkey == "file") {
                cfg.flat_file = val;
            } else {
                ButtonDef btn;
                btn.label = key;
                int comma = 0;
                while (true) {
                    int next = val.indexOf(',', comma);
                    String s = (next == -1) ? val.substring(comma) : val.substring(comma, next);
                    s.trim();
                    if (s.length() > 0) btn.search.push_back(s);
                    if (next == -1) break;
                    comma = next + 1;
                }
                if (btn.search.size() > 0) cfg.buttons.push_back(btn);
            }
        }
    }
    f.close();
    return true;
}

struct Category {
    String name;
    String dir_path;
};

static bool has_brand_folders(FS &fs, const String &dir) {
    if (dir.length() == 0) return false;
    File root = fs.open(dir);
    if (!root || !root.isDirectory()) return false;
    bool found = false;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        if (fullPath == "") break;
        if (isDir) {
            found = true;
            break;
        }
    }
    root.close();
    return found;
}

static std::vector<Category> discover_categories(FS &fs) {
    std::vector<Category> cats;
    String roots[2] = {g_ir_root + "/assets", g_ir_root};
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
                    if (c.name.equalsIgnoreCase(name)) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) cats.push_back({name, fullPath});
            }
        }
        root.close();
    }

    const char *virtuals[][2] = {
        {"TVs", "tv.ir"},          {"ACs", "ac.ir"},
        {"LED_Lighting", "leds.ir"}, {"Projectors", "projectors.ir"},
        {"Audio", "audio.ir"},     {"Fans", "fans.ir"},
    };
    const int virtual_count = (int)(sizeof(virtuals) / sizeof(virtuals[0]));
    for (int i = 0; i < virtual_count; i++) {
        if (resolve_flat(fs, virtuals[i][1]).length() == 0) continue;
        String lower = virtuals[i][0];
        lower.toLowerCase();
        bool dup = false;
        for (auto &c : cats) {
            String cl = c.name;
            cl.toLowerCase();
            if (cl.startsWith(lower)) {
                dup = true;
                break;
            }
        }
        if (!dup) cats.push_back({virtuals[i][0], ""});
    }
    return cats;
}

static void open_category(FS &fs, const Category &cat, const CategoryConfig &cfg) {
    String lower = cat.name;
    lower.toLowerCase();

    std::vector<ButtonDef> buttons = cfg.buttons;
    if (buttons.empty()) buttons = layout_for(lower);

    String flat = cfg.flat_file;
    if (flat.length() == 0) flat = builtin_flat_for(lower);
    String flat_path = resolve_flat(fs, flat);
    String brands_path = has_brand_folders(fs, cat.dir_path) ? cat.dir_path : "";

    if (flat_path.length() > 0) {
        SigIndex idx = build_flat_index(fs, flat_path);
        if (!idx.empty()) {
            if (buttons.empty()) {
                generic_signal_list(fs, idx, cat.name, cat.name, brands_path, cfg.orientation);
            } else {
                show_remote(fs, idx, buttons, cat.name, cat.name, brands_path, cfg.orientation);
            }
            return;
        }
    }

    if (brands_path.length() > 0) {
        show_brands_flow(fs, cat.dir_path, buttons, cfg.orientation, cat.name);
        return;
    }

    if (!buttons.empty()) {
        SigIndex idx = build_dir_index(fs, cat.dir_path);
        if (!idx.empty()) {
            show_remote(fs, idx, buttons, cat.name, cat.name, "", cfg.orientation);
        } else {
            displayError("No .ir files found");
            delay(1500);
        }
        return;
    }

    displayError("No IR content found");
    delay(1500);
}

static void show_categories() {
    FS *fsPtr = nullptr;
#if defined(UNIVERSAL_IR_LITTLEFS_ONLY)
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

    g_ir_root = find_db_root(fs, "UniversalIR");

    if (!fs.exists(g_ir_root)) {
        fs.mkdir(g_ir_root);
#if defined(UNIVERSAL_IR_LITTLEFS_ONLY)
        displayError("Put IR files in /UniversalIR (LittleFS)");
#else
        displayError("Put IR files in /UniversalIR (SD or LittleFS)");
#endif
        delay(2000);
        return;
    }

    std::map<String, CategoryConfig> configs;
    load_layouts(fs, configs);

    std::vector<Category> cats = discover_categories(fs);

    if (cats.empty()) {
        displayError("No category folders found");
        delay(2000);
        return;
    }

    int globalRot = bruceConfigPins.rotation;
    int irRot = load_ir_orient(fs, globalRot);
    if (irRot != globalRot) apply_display_orientation(irRot);

    returnToMenu = false;
    while (!returnToMenu) {
        options.clear();
        for (auto &cat : cats) {
            String name = cat.name;
            options.push_back({name.c_str(), [&fs, &configs, cat]() {
                String key = cat.name;
                key.toLowerCase();
                CategoryConfig cfg;
                if (configs.count(key) > 0) cfg = configs[key];
                open_category(fs, cat, cfg);
            }});
        }
        options.push_back({"Main Menu", [&]() { returnToMenu = true; }});
        loopOptions(options);
    }
    options.clear();

    save_ir_orient(fs, bruceConfigPins.rotation);
    if (bruceConfigPins.rotation != globalRot) {
        apply_display_orientation(globalRot);
        bruceConfigPins.saveFile();
    }
}

void universalIRcodes() {
    show_categories();
}
