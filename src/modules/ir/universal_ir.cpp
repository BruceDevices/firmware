#include "universal_ir.h"
#include "TV-B-Gone.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "custom_ir.h"
#include "ir_read.h"
#include "universal_history.h"

#include <functional>
#include <map>

#define UNIVERSAL_IR_ROOT "/UniversalIR"
#define UNIVERSAL_IR_ASSETS "/UniversalIR/assets"
#define ORIENT_FILE "/UniversalIR/orient.txt"

#define HIST_RECENT_CAP 24
#define HIST_FAV_CAP 64

// Long-press (hold SEL) threshold for the Favorites toggle.
#define LONG_PRESS_MS 750

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

// Describes where a SigIndex came from, so a signal can be replayed later from
// the Favorites/Recent lists (path is a folder -> index recursively, or a
// single .ir file).
struct IrSource {
    String path;
    bool isDir = false;
};

static String starless(const String &s) {
    return s.startsWith("★ ") ? s.substring(2) : s;
}

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
    int availH = tftHeight - HEADER - 26;
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

    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setTextFont(FP);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("OK=sel Hold=star ESC=back", tftWidth / 2, tftHeight - 13);

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

// Built-in IR signals — works with no files on disk (like RF Generic / TV-B-Gone).
// Common power codes for the most widespread TV protocols.
static void ir_grid_navigate(
    const std::vector<String> &labels, const String &title,
    const std::function<bool(int)> &on_select, const std::function<void(int)> &on_long
);

// ── Built-in IR: signal table type ────────────────────────────────────
struct BuiltInSig {
    const char *proto;
    const char *addr;
    const char *cmd;
    uint8_t bits;
};

static void _sendBuiltinSig(const BuiltInSig &s) {
    IRCode code(s.proto, s.addr, s.cmd, "", s.bits);
    sendIRCommand(&code, true);
}

// ── A "group" = one function (e.g. Power) with all brands' codes ─────
struct BuiltinGroup {
    const char *label;       // "Power", "Vol+", etc.
    const BuiltInSig *sigs;
    uint8_t count;
};

// ── Grid: show generic buttons, each sends ALL brands for that function ──
static void _builtinFuncGrid(const char *title, const BuiltinGroup *groups, int groupCount) {
    std::vector<String> labels;
    for (int i = 0; i < groupCount; i++) labels.push_back(groups[i].label);
    labels.push_back("Back");

    ir_grid_navigate(labels, title, [title, groups, groupCount](int sel) -> bool {
        if (sel >= groupCount) return true;
        const BuiltinGroup &g = groups[sel];
        tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
        drawMainBorderWithTitle(title);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextFont(FM);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(g.label, tftWidth / 2, tftHeight / 2 - 30);
        tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
        tft.setTextFont(FP);
        tft.drawString("ESC to cancel", tftWidth / 2, tftHeight / 2 + 15);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextFont(FM);
        for (int i = 0; i < g.count; i++) {
            tft.fillRect(0, tftHeight / 2 - 10, tftWidth, 20, bruceConfig.bgColor);
            tft.drawString(String(i + 1) + " / " + String(g.count), tftWidth / 2, tftHeight / 2 - 5);
            _sendBuiltinSig(g.sigs[i]);
            delay(300);
            if (check(EscPress)) break;
        }
        return false;
    }, nullptr);
}

// ═══════════════════════════════════════════════════════════════════════
//  TVs — each button sends ALL brands' codes
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _tvPower[] = {
    {"Samsung32","07","02",32},{"NECext","04","08",32},{"SIRC","01","15",12},
    {"RC5","00","0C",12},{"Kaseikyo","02","3D",48},{"NEC","40","09",32},
    {"NEC","00","B7",32},{"NEC","04","08",32},{"NECext","20","08",32},
};
static const BuiltInSig _tvVolUp[] = {
    {"Samsung32","07","07",32},{"NECext","04","02",32},{"SIRC","01","12",12},
    {"RC5","00","10",12},{"Kaseikyo","02","20",48},{"NEC","40","1A",32},
    {"NEC","00","B4",32},{"NEC","04","02",32},{"NECext","20","02",32},
};
static const BuiltInSig _tvVolDn[] = {
    {"Samsung32","07","0B",32},{"NECext","04","03",32},{"SIRC","01","13",12},
    {"RC5","00","11",12},{"Kaseikyo","02","21",48},{"NEC","40","1E",32},
    {"NEC","00","B5",32},{"NEC","04","03",32},{"NECext","20","03",32},
};
static const BuiltInSig _tvMute[] = {
    {"Samsung32","07","0F",32},{"NECext","04","06",32},{"SIRC","01","14",12},
    {"RC5","00","0D",12},{"Kaseikyo","02","32",48},{"NEC","40","0D",32},
    {"NEC","00","B6",32},{"NEC","04","06",32},{"NECext","20","06",32},
};
static const BuiltInSig _tvChUp[] = {
    {"Samsung32","07","12",32},{"NECext","04","00",32},{"SIRC","01","10",12},
    {"RC5","00","20",12},{"Kaseikyo","02","10",48},{"NEC","40","1B",32},
};
static const BuiltInSig _tvChDn[] = {
    {"Samsung32","07","10",32},{"NECext","04","01",32},{"SIRC","01","11",12},
    {"RC5","00","21",12},{"Kaseikyo","02","11",48},{"NEC","40","1F",32},
};

static const BuiltinGroup _tvGroups[] = {
    {"Power",  _tvPower,  9},
    {"Vol+",   _tvVolUp,  9},
    {"Vol-",   _tvVolDn,  9},
    {"Mute",   _tvMute,   9},
    {"Ch+",    _tvChUp,   6},
    {"Ch-",    _tvChDn,   6},
};

// ═══════════════════════════════════════════════════════════════════════
//  AC
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _acPower[]  = {{"NEC","00","B7",32}};
static const BuiltInSig _acMode[]   = {{"NEC","00","B8",32}};
static const BuiltInSig _acTempUp[] = {{"NEC","00","B4",32}};
static const BuiltInSig _acTempDn[] = {{"NEC","00","B5",32}};
static const BuiltInSig _acFanUp[]  = {{"NEC","00","B2",32}};
static const BuiltInSig _acFanDn[]  = {{"NEC","00","B3",32}};
static const BuiltInSig _acSwing[]  = {{"NEC","00","B6",32}};

static const BuiltinGroup _acGroups[] = {
    {"Power", _acPower, 1},{"Mode", _acMode, 1},{"Temp+", _acTempUp, 1},
    {"Temp-", _acTempDn, 1},{"Fan+", _acFanUp, 1},{"Fan-", _acFanDn, 1},
    {"Swing", _acSwing, 1},
};

// ═══════════════════════════════════════════════════════════════════════
//  Audio
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _audPower[] = {
    {"NEC","40","0B",32},{"NECext","10","80",32},{"NEC","20","1A",32},
    {"SIRC","10","15",12},{"Samsung32","07","02",32},{"NEC","0E","1C",32},
};
static const BuiltInSig _audVolUp[] = {
    {"NEC","40","1A",32},{"NECext","10","81",32},{"NEC","20","18",32},
    {"SIRC","10","12",12},{"Samsung32","07","07",32},{"NEC","0E","10",32},
};
static const BuiltInSig _audVolDn[] = {
    {"NEC","40","1E",32},{"NECext","10","82",32},{"NEC","20","19",32},
    {"SIRC","10","13",12},{"Samsung32","07","0B",32},{"NEC","0E","11",32},
};
static const BuiltInSig _audMute[] = {
    {"NEC","40","0D",32},{"NECext","10","83",32},{"NEC","20","1B",32},
    {"SIRC","10","14",12},{"Samsung32","07","0F",32},{"NEC","0E","14",32},
};

static const BuiltinGroup _audGroups[] = {
    {"Power", _audPower, 6},{"Vol+", _audVolUp, 6},
    {"Vol-", _audVolDn, 6},{"Mute", _audMute, 6},
};

// ═══════════════════════════════════════════════════════════════════════
//  Fans
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _fanPower[]  = {{"NEC","00","B7",32}};
static const BuiltInSig _fanSpeed[]  = {{"NEC","00","B4",32}};
static const BuiltInSig _fanSpeedD[] = {{"NEC","00","B5",32}};
static const BuiltInSig _fanSwing[]  = {{"NEC","00","B6",32}};

static const BuiltinGroup _fanGroups[] = {
    {"Power", _fanPower, 1},{"Speed+", _fanSpeed, 1},
    {"Speed-", _fanSpeedD, 1},{"Swing", _fanSwing, 1},
};

// ═══════════════════════════════════════════════════════════════════════
//  LED Lighting
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _ledPowerOn[]  = {{"NEC","00","40",32}};
static const BuiltInSig _ledPowerOff[] = {{"NEC","00","19",32}};
static const BuiltInSig _ledRed[]      = {{"NEC","00","16",32}};
static const BuiltInSig _ledGreen[]    = {{"NEC","00","1A",32}};
static const BuiltInSig _ledBlue[]     = {{"NEC","00","11",32}};
static const BuiltInSig _ledWhite[]    = {{"NEC","00","15",32}};
static const BuiltInSig _ledBrUp[]     = {{"NEC","00","47",32}};
static const BuiltInSig _ledBrDn[]     = {{"NEC","00","44",32}};
static const BuiltInSig _ledSmooth[]   = {{"NEC","00","1B",32}};
static const BuiltInSig _ledFlash[]    = {{"NEC","00","13",32}};

static const BuiltinGroup _ledGroups[] = {
    {"Power On",  _ledPowerOn,  1},{"Power Off", _ledPowerOff, 1},
    {"Red",       _ledRed,      1},{"Green",     _ledGreen,    1},
    {"Blue",      _ledBlue,     1},{"White",     _ledWhite,    1},
    {"Bright+",   _ledBrUp,     1},{"Bright-",   _ledBrDn,     1},
    {"Smooth",    _ledSmooth,   1},{"Flash",     _ledFlash,    1},
};

// ═══════════════════════════════════════════════════════════════════════
//  Projectors
// ═══════════════════════════════════════════════════════════════════════
static const BuiltInSig _projPower[] = {
    {"NEC","04","08",32},{"NEC","00","0B",32},{"NEC","01","08",32},
    {"SIRC","14","15",12},
};
static const BuiltInSig _projVolUp[] = {
    {"NEC","04","02",32},{"NEC","00","10",32},{"NEC","01","02",32},
    {"SIRC","14","12",12},
};
static const BuiltInSig _projVolDn[] = {
    {"NEC","04","03",32},{"NEC","00","11",32},{"NEC","01","03",32},
    {"SIRC","14","13",12},
};
static const BuiltInSig _projMute[] = {
    {"NEC","04","06",32},{"NEC","00","14",32},{"SIRC","14","14",12},
};

static const BuiltinGroup _projGroups[] = {
    {"Power", _projPower, 4},{"Vol+", _projVolUp, 4},
    {"Vol-", _projVolDn, 4},{"Mute", _projMute, 3},
};

// ═══════════════════════════════════════════════════════════════════════
//  Entry points
// ═══════════════════════════════════════════════════════════════════════
static void _tvBuiltin()       { _builtinFuncGrid("TVs",          _tvGroups,  6); }
static void _acBuiltin()       { _builtinFuncGrid("AC",           _acGroups,  7); }
static void _audioBuiltin()    { _builtinFuncGrid("Audio",        _audGroups, 4); }
static void _fansBuiltin()     { _builtinFuncGrid("Fans",         _fanGroups, 4); }
static void _ledBuiltin()      { _builtinFuncGrid("LED Lighting", _ledGroups, 10); }
static void _projectorBuiltin(){ _builtinFuncGrid("Projectors",   _projGroups, 4); }

static void show_brands_flow(
    FS &fs, String cat_path, const std::vector<ButtonDef> &buttons, int orientation, String title
);

// Vertical navigation across pages: Up/Down move by column and stay on the
// same page unless at an edge, where they flow to the adjacent page. Wrap to
// the other end only at the very first/last cell, like the rest of Bruce.
static int grid_move_up(int sel, int total, const GridMetrics &m) {
    int col = sel % m.cols;
    int row = sel / m.cols;
    if (row > 0) return sel - m.cols;
    int page = sel / m.perPage;
    if (page > 0) {
        int prevLast = page * m.perPage - 1;
        int t = prevLast - (prevLast % m.cols) + col;
        return (t < total) ? t : total - 1;
    }
    if (col == 0) return total - 1;
    int lastRowFirst = (total - 1) - ((total - 1) % m.cols);
    int t = lastRowFirst + col;
    return (t < total) ? t : total - 1;
}

static int grid_move_down(int sel, int total, const GridMetrics &m) {
    int row = sel / m.cols;
    int nsel = sel + m.cols;
    if (row < m.rows - 1) return (nsel < total) ? nsel : total - 1;
    if (sel == total - 1) return 0;
    return (nsel < total) ? nsel : total - 1;
}

// Generic paged grid over plain labels (mirrors the RF module's grid_navigate).
// Short SEL -> on_select(sel) (return true to exit), HOLD SEL (~750ms) ->
// on_long(sel). Used by the Favorites/Recent lists.
static void ir_grid_navigate(
    const std::vector<String> &labels, const String &title,
    const std::function<bool(int)> &on_select, const std::function<void(int)> &on_long = nullptr
) {
    if (labels.empty()) return;
    std::vector<ButtonDef> btns;
    btns.reserve(labels.size());
    for (const auto &l : labels) {
        ButtonDef b;
        b.label = l;
        btns.push_back(b);
    }

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
                sel = grid_move_up(sel, total, m);
                moved = true;
            }
            if (check(DownPress)) {
                sel = grid_move_down(sel, total, m);
                moved = true;
            }
            if (check(NextPagePress)) {
                sel = (sel + m.perPage) % total;
                moved = true;
            }
            if (check(PrevPagePress)) {
                int np = (sel - m.perPage) % total;
                if (np < 0) np += total;
                sel = np;
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

        // Short tap = select, hold ~750ms = long press (on_long). Some boards
        // re-set SelPress every ~200ms while the button is held, so the press is
        // considered released once SelPress stays clear for 60ms.
        if (millis() - openTs > 600 && SelPress) {
            unsigned long firstSeen = millis();
            unsigned long lastSeen = firstSeen;
            bool escaped = false;
            while (millis() - lastSeen < 60) {
                if (SelPress) {
                    lastSeen = millis();
                    SelPress = false;
                    AnyKeyPress = false;
                    SerialCmdPress = false;
                }
                if (EscPress) {
                    escaped = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            if (escaped) {
                EscPress = false;
                break;
            }
            bool isLong = (lastSeen - firstSeen >= LONG_PRESS_MS);
            if (isLong) {
                if (on_long != nullptr) on_long(sel);
                drawMainBorderWithTitle(title);
                render_page(m, btns, total, page, sel);
                openTs = millis();
            } else {
                if (on_select(sel)) break;
                drawMainBorderWithTitle(title);
                render_page(m, btns, total, page, sel);
                openTs = millis();
            }
        }
    }
}

static bool remote_grid(
    FS &fs, const SigIndex &idx, const std::vector<ButtonDef> &buttons, const String &title,
    const String &spam_brand, const String &brands_path, int orientation, IrSource src = IrSource()
) {
    std::vector<ButtonDef> btns = buttons;
    std::vector<HistEntry> favs = (src.path.length() > 0)
        ? hist_load(fs, g_ir_root + "/favorites.txt")
        : std::vector<HistEntry>();
    for (auto &b : btns) {
        if (src.path.length() > 0 && hist_has(favs, src.path, b.label)) b.label = "★ " + b.label;
    }
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
                sel = grid_move_up(sel, total, m);
                moved = true;
            }
            if (check(DownPress)) {
                sel = grid_move_down(sel, total, m);
                moved = true;
            }
            if (check(NextPagePress)) {
                sel = (sel + m.perPage) % total;
                moved = true;
            }
            if (check(PrevPagePress)) {
                int np = (sel - m.perPage) % total;
                if (np < 0) np += total;
                sel = np;
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

        // Short tap = select, hold ~750ms = toggle Favorites. Some boards
        // re-set SelPress every ~200ms while the button is held, so the press is
        // considered released once SelPress stays clear for 60ms.
        if (millis() - openTs > 600 && SelPress) {
            unsigned long firstSeen = millis();
            unsigned long lastSeen = firstSeen;
            bool escaped = false;
            while (millis() - lastSeen < 60) {
                if (SelPress) {
                    lastSeen = millis();
                    SelPress = false;
                    AnyKeyPress = false;
                    SerialCmdPress = false;
                }
                if (EscPress) {
                    escaped = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            if (escaped) {
                EscPress = false;
                break;
            }
            bool isLong = (lastSeen - firstSeen >= LONG_PRESS_MS);
            ButtonDef &btn = btns[sel];
            if (isLong) {
                if (sel < (int)buttons.size() && src.path.length() > 0) {
                    String clean = starless(btn.label);
                    std::vector<HistEntry> favs = hist_load(fs, g_ir_root + "/favorites.txt");
                    if (hist_has(favs, src.path, clean)) {
                        hist_remove(favs, src.path, clean);
                        displayWarning("Removed from Favorites");
                    } else {
                        hist_add(favs, src.path, clean, src.isDir, HIST_FAV_CAP);
                        displaySuccess("Added to Favorites");
                    }
                    hist_save(fs, g_ir_root + "/favorites.txt", favs);
                    btn.label = (hist_has(favs, src.path, clean) ? "★ " : "") + clean;
                    delay(600);
                }
                drawMainBorderWithTitle(title);
                render_page(m, btns, total, page, sel);
                openTs = millis();
            } else if (btn.label == "Back") {
                break;
            } else if (btn.label == "Orient") {
                gsetRotation(true);
                returnToMenu = false;
                return true;
            } else if (btn.label == "Brands") {
                show_brands_flow(fs, brands_path, buttons, orientation, title);
                drawMainBorderWithTitle(title);
                render_page(m, btns, total, page, sel);
                openTs = millis();
                continue;
            } else {
                int sent = spam_index(fs, idx, btn.search, spam_brand, btn.label);
                if (sent > 0 && src.path.length() > 0) {
                    std::vector<HistEntry> rec = hist_load(fs, g_ir_root + "/recent.txt");
                    hist_add(rec, src.path, starless(btn.label), src.isDir, HIST_RECENT_CAP);
                    hist_save(fs, g_ir_root + "/recent.txt", rec);
                }
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
    }
    return false;
}

static void show_remote(
    FS &fs, const SigIndex &idx, const std::vector<ButtonDef> &buttons, String title, String spam_brand,
    String brands_path, int orientation, IrSource src = IrSource()
) {
    (void)orientation;
    while (true) {
        bool rotated = remote_grid(fs, idx, buttons, title, spam_brand, brands_path, ORIENT_GRID, src);
        if (!rotated) break;
    }
}

static void generic_signal_list(
    FS &fs, const SigIndex &idx, String brand, String title, String brands_path, int orientation,
    IrSource src = IrSource()
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
            options.push_back({label.c_str(), [&fs, &idx, brand, sig_name, src]() {
                std::vector<String> search;
                search.push_back(sig_name);
                int sent = spam_index(fs, idx, search, brand, sig_name);
                if (sent > 0 && src.path.length() > 0) {
                    std::vector<HistEntry> rec = hist_load(fs, g_ir_root + "/recent.txt");
                    hist_add(rec, src.path, sig_name, src.isDir, HIST_RECENT_CAP);
                    hist_save(fs, g_ir_root + "/recent.txt", rec);
                }
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
                IrSource src;
                src.path = brand_path;
                src.isDir = true;
                if (buttons.empty()) {
                    generic_signal_list(fs, idx, brand, brand, "", orientation, src);
                } else {
                    show_remote(fs, idx, buttons, brand, brand, "", orientation, src);
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
            if (cl.startsWith(lower) || lower.startsWith(cl)) {
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
            IrSource src;
            src.path = flat_path;
            src.isDir = false;
            if (buttons.empty()) {
                generic_signal_list(fs, idx, cat.name, cat.name, brands_path, cfg.orientation, src);
            } else {
                show_remote(fs, idx, buttons, cat.name, cat.name, brands_path, cfg.orientation, src);
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
            IrSource src;
            src.path = cat.dir_path;
            src.isDir = true;
            show_remote(fs, idx, buttons, cat.name, cat.name, "", cfg.orientation, src);
        } else {
            displayError("No .ir files found");
            delay(1500);
        }
        return;
    }

    displayError("No IR content found");
    delay(1500);
}

// Grid over a persisted history list (Favorites/Recent). Short SEL replays the
// signal (rebuilding its index from the stored path); HOLDING SEL removes it.
static void ir_history_grid(FS &fs, const String &file, const String &title, bool favMode) {
    std::vector<HistEntry> list = hist_load(fs, file);
    if (list.empty()) {
        displayError("List is empty");
        delay(1200);
        return;
    }

    std::vector<String> labels;
    for (const auto &e : list) labels.push_back((favMode ? "★ " : "") + e.name);
    labels.push_back("Back");

    ir_grid_navigate(
        labels,
        title,
        [&fs, &list, &labels](int sel) {
            if (sel >= (int)list.size()) return true;
            SigIndex idx = list[sel].isDir ? build_dir_index(fs, list[sel].path)
                                           : build_flat_index(fs, list[sel].path);
            if (idx.empty()) {
                displayError("Signal not found");
                delay(1200);
                return false;
            }
            std::vector<String> search;
            search.push_back(list[sel].name);
            String clean = starless(labels[sel]);
            int sent = spam_index(fs, idx, search, clean, clean);
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
            return false;
        },
        [&fs, &list, &file](int sel) {
            if (sel >= (int)list.size()) return;
            std::vector<HistEntry> cur = hist_load(fs, file);
            hist_remove(cur, list[sel].path, list[sel].name);
            hist_save(fs, file, cur);
            displayWarning("Entry removed");
            delay(600);
        }
    );
}

// ---- IR Clone into the DB --------------------------------------------------
// Captures a remote button (headless IR read) and saves it as a normal .ir file
// inside a DB category, so the cloned button shows up in the browser and can be
// replayed with the standard pipeline.

struct CloneArgs {
    String *out;
    volatile bool *abort;
    volatile bool *done;
};

static void ir_clone_task(void *p) {
    CloneArgs *a = (CloneArgs *)p;
    String captured;
    {
        // RAII: ~IrRead() runs disableIRIn() + frees the IRrecv timer before the
        // task self-deletes (same pattern as the dual RF+IR detector).
        IrRead reader(true, true);
        captured = reader.loop_headless(30, a->abort);
    }
    *a->out = captured;
    *a->done = true;
    vTaskDelete(NULL);
}

static void ir_clone_flow(FS &fs) {
    // 1) Capture signal FIRST (ESC aborts).
    tft.fillRect(0, 0, tftWidth, tftHeight, bruceConfig.bgColor);
    drawMainBorderWithTitle("Clone IR");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextFont(FM);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Point the remote at the IR receiver", tftWidth / 2, tftHeight / 2 - 30);
    tft.drawString("and press the button to clone.", tftWidth / 2, tftHeight / 2 - 12);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setTextFont(FP);
    tft.setTextSize(1);
    tft.drawString("ESC to cancel", tftWidth / 2, tftHeight / 2 + 10);

    volatile bool abort = false;
    volatile bool done = false;
    String captured;
    CloneArgs args{&captured, &abort, &done};
    TaskHandle_t task = NULL;
    if (xTaskCreate(ir_clone_task, "irclone", 16384, &args, 2, &task) != pdPASS) {
        displayError("Failed to start capture");
        delay(1500);
        return;
    }
    while (!done) {
        if (check(EscPress)) abort = true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (task != NULL) vTaskDelete(task);

    if (captured.length() == 0) {
        displayError("No signal captured");
        delay(1500);
        return;
    }

    // 2) Signal (button) name.
    String sig = keyboard("Signal", 30, "Signal name:");
    if (sig == "\x1B" || sig.length() == 0) return;
    sig.trim();
    if (sig.length() == 0) {
        displayError("Invalid name");
        delay(1500);
        return;
    }

    // 3) Destination: existing folder categories or a brand-new folder.
    std::vector<Category> cats = discover_categories(fs);
    String target = "";
    {
        options.clear();
        for (auto &c : cats) {
            if (c.dir_path.length() == 0) continue;
            String n = c.name;
            options.push_back({n.c_str(), [&target, path = c.dir_path]() { target = path; }});
        }
        options.push_back({"New folder", [&]() { target = "__new__"; }});
        options.push_back({"Cancel", [&]() { target = ""; }});
        loopOptions(options, MENU_TYPE_SUBMENU, "Save to category");
    }
    if (target == "") return;

    if (target == "__new__") {
        String folder = keyboard("NewFolder", 30, "Folder name:");
        if (folder == "\x1B" || folder.length() == 0) return;
        String clean;
        for (unsigned int i = 0; i < folder.length(); i++) {
            char ch = folder[i];
            if (isalnum(ch) || ch == '_' || ch == '-' || ch == ' ' || ch == '/') clean += ch;
        }
        clean.trim();
        if (clean.length() == 0) {
            displayError("Invalid name");
            delay(1500);
            return;
        }
        target = g_ir_root + "/" + clean;
        if (!fs.exists(target)) fs.mkdir(target);
    }

    // 4) Unique file path.
    String path = target + "/" + sig + ".ir";
    int n = 1;
    while (fs.exists(path)) {
        path = target + "/" + sig + "_" + String(n) + ".ir";
        n++;
    }

    // 5) Store the capture.
    String content = captured;
    content.replace("name: Unknown", "name: " + sig);
    File f = fs.open(path, FILE_WRITE);
    if (!f) {
        displayError("Error writing file");
        delay(1500);
        return;
    }
    f.print(content);
    f.close();
    displaySuccess("Cloned to " + path.substring(g_ir_root.length() + 1));
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
    bool hasDB = fs.exists(g_ir_root);
    std::map<String, CategoryConfig> configs;
    int globalRot = bruceConfigPins.rotation;
    if (hasDB) {
        load_layouts(fs, configs);
        int irRot = load_ir_orient(fs, globalRot);
        if (irRot != globalRot) apply_display_orientation(irRot);
    }

    returnToMenu = false;
    while (!returnToMenu) {
        options.clear();

        if (hasDB) {
            // Quick access lists + clone into DB (persisted on same FS as DB)
            String favFile = g_ir_root + "/favorites.txt";
            String recFile = g_ir_root + "/recent.txt";
            std::vector<HistEntry> favs = hist_load(fs, favFile);
            std::vector<HistEntry> recs = hist_load(fs, recFile);
            options.push_back({"★ Favorites (" + String(favs.size()) + ")", [&fs, favFile]() {
                ir_history_grid(fs, favFile, "Favorites", true);
            }});
            options.push_back({"Recent (" + String(recs.size()) + ")", [&fs, recFile]() {
                ir_history_grid(fs, recFile, "Recent", false);
            }});
            options.push_back({"Clone IR -> DB", [&fs]() {
                ir_clone_flow(fs);
            }});

            std::vector<Category> cats = discover_categories(fs);
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
        }

        // Built-in categories — always add, skip if DB already has same name
        {
            struct Builtin { const char *name; std::function<void()> fn; };
            Builtin builtins[] = {
                {"TVs",          []() { _tvBuiltin(); }},
                {"AC",           []() { _acBuiltin(); }},
                {"Audio",        []() { _audioBuiltin(); }},
                {"Fans",         []() { _fansBuiltin(); }},
                {"LED Lighting", []() { _ledBuiltin(); }},
                {"Projectors",   []() { _projectorBuiltin(); }},
            };
            for (auto &b : builtins) {
                bool exists = false;
                String bLower = String(b.name);
                bLower.toLowerCase();
                for (auto &o : options) {
                    String oLower = String(o.label);
                    oLower.toLowerCase();
                    // Match exact or prefix (e.g. "ACs" matches "AC", "TVs" matches "TV")
                    if (oLower == bLower || oLower.startsWith(bLower) || bLower.startsWith(oLower)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) options.push_back({b.name, b.fn});
            }
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
