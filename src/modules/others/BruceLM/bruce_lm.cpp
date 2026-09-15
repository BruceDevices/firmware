/**
 * @file bruce_lm.cpp
 * @brief "Others -> BruceLM" module: on-device LLM.
 *
 *  ---- Made by @Doominator1 on GitHub, Jul 2026
 *  ---- Much thanks to @karpathy for the base of this project; llama2.c!
 *
 * Models + tokenizers live on SD at /BruceLM/models/ (auto-created here,
 * matching the same idiom used by ir_read.cpp's /BruceIR and
 * wardriving.cpp's /BruceWardriving). Model selection reuses Bruce's
 * existing loopSD() file picker; text entry reuses the existing keyboard()
 * overlay. Everything is drawn with the shared theme colors/border helpers
 * so it matches the rest of the firmware.
 *
 * Layout (top to bottom, inside the standard Bruce border+title):
 *   - scrolling chat transcript, word-wrapped, "You:" lines in the primary
 *     theme color and assistant text in the secondary color
 *   - a horizontal separator line
 *   - a one-line "input bar" row showing the current/last prompt
 *   - a footer hint row ("OK: new prompt   Esc: exit")
 *
 * The whole frame is redrawn from scratch on every change (new token,
 * scroll tick, new turn) instead of incrementally patching the screen -
 * slightly more pixels pushed, but it removes an entire class of stale-text/
 * overlap bugs that incremental cursor-based drawing is prone to.
 *
 * Scrolling uses this board's rotary encoder, which the board's InputHandler
 * (boards/lilygo-t-embed-cc1101/interface.cpp) maps to NextPress/PrevPress -
 * a separate signal from the physical Select/Back buttons (SelPress/EscPress),
 * so scrolling and cancelling never conflict. NextPress scrolls down toward
 * newer content, PrevPress scrolls up toward older content, matching how
 * Next/Prev already mean forward/backward everywhere else in this firmware.
 */
#include "bruce_lm.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "llm_engine.h"
#include <ArduinoJson.h>
#include <globals.h>
#include <vector>

using namespace bruce_llm;

namespace {

constexpr const char *kRootDir = "/BruceLM";
constexpr const char *kModelsDir = "/BruceLM/models";
constexpr const char *kConfigPath = "/BruceLM/config.json";

// Seed presets cycled through with Next/Prev on the Seed row, rather than
// free-form numeric entry - "Random" reseeds from the hardware RNG each
// reply (value 0 is the sentinel llm_engine.cpp treats as "reseed"); the
// rest give byte-for-byte reproducible output for the same prompt.
struct SeedPreset {
    const char *label;
    uint32_t value;
};
constexpr SeedPreset kSeedPresets[] = {
    {"1",      1   },
    {"2",      2   },
    {"3",      3   },
    {"4",      4   },
    {"5",      5   },
    {"6",      6   },
    {"7",      7   },
    {"8",      8   },
    {"9",      9   },
    {"10",     10  },
    {"42",     42  },
    {"67",     67  },
    {"100",    100 },
    {"1000",   1000},
    {"Random", 0   },
};
constexpr int kNumSeedPresets = sizeof(kSeedPresets) / sizeof(kSeedPresets[0]);
constexpr int kDefaultSeedPresetIndex = kNumSeedPresets - 1; // "Random"

// Every field here is a real, functioning knob on the engine
// (llm_engine.cpp's generate() / GenerationParams) - nothing decorative.
// Defaults are tuned for a model this small (3.3M params) rather than copied
// from typical LLM defaults - see git history/PR for the prompt-comparison
// data behind the specific values.
//
// Wraps prompts as `userTag + prompt + "\n" + botTag` before encoding, so a
// chat-finetuned model (trained on that exact "<user>: ...\n<bot>: ..." pair
// format) is cued to answer immediately instead of hallucinating a whole
// fake turn. On by default to match Chat3.3M.bin, the default recommended
// model - plain story models (e.g. stories260K.bin) expect raw text instead.
struct BruceLMSettings {
    float temperature = 0.5f;
    float topP = 0.6f;
    float repetitionPenalty = 1.15f;
    int maxTokens = 128;
    int seedPresetIndex = kDefaultSeedPresetIndex;
    bool chatTemplateEnabled = true;
    String userTag = "<user>: ";
    String botTag = "<bot>:";

    uint32_t seed() const { return kSeedPresets[seedPresetIndex].value; }
};

BruceLMSettings loadSettings(FS &fs) {
    BruceLMSettings s;
    if (!fs.exists(kConfigPath)) return s;
    File f = fs.open(kConfigPath, FILE_READ);
    if (!f) return s;
    JsonDocument doc;
    if (!deserializeJson(doc, f)) {
        if (!doc["temperature"].isNull()) s.temperature = doc["temperature"];
        if (!doc["topP"].isNull()) s.topP = doc["topP"];
        if (!doc["repetitionPenalty"].isNull()) s.repetitionPenalty = doc["repetitionPenalty"];
        if (!doc["maxTokens"].isNull()) s.maxTokens = doc["maxTokens"];
        if (!doc["seedPresetIndex"].isNull()) {
            int idx = doc["seedPresetIndex"];
            if (idx >= 0 && idx < kNumSeedPresets) s.seedPresetIndex = idx;
        }
        if (!doc["chatTemplateEnabled"].isNull()) s.chatTemplateEnabled = doc["chatTemplateEnabled"];
        if (!doc["userTag"].isNull()) s.userTag = doc["userTag"].as<String>();
        if (!doc["botTag"].isNull()) s.botTag = doc["botTag"].as<String>();
    }
    f.close();
    return s;
}

void saveSettings(FS &fs, const BruceLMSettings &s) {
    JsonDocument doc;
    doc["temperature"] = s.temperature;
    doc["topP"] = s.topP;
    doc["repetitionPenalty"] = s.repetitionPenalty;
    doc["maxTokens"] = s.maxTokens;
    doc["seedPresetIndex"] = s.seedPresetIndex;
    doc["chatTemplateEnabled"] = s.chatTemplateEnabled;
    doc["userTag"] = s.userTag;
    doc["botTag"] = s.botTag;
    File f = fs.open(kConfigPath, FILE_WRITE);
    if (!f) return;
    serializeJsonPretty(doc, f);
    f.close();
}

// All of this is deliberately expressed in terms of existing global layout
// constants (BORDER_PAD_X/Y, FP/FM, LW/LH) rather than magic numbers, and the
// outer rounded-rect border is drawn at (5,5,tftWidth-10,tftHeight-10) by
// drawStatusBar() - kBottomMargin/kTopGap keep our own content clear of it.
constexpr int kTopGap = 6;
constexpr int kBottomMargin = 6;
constexpr int kFooterH = 16;
constexpr int kInputBarH = 16;
constexpr int kSepGap = 4;

struct ChatGeometry {
    int outputTop;
    int sepY;
    int inputBarY;
    int footerY;
    int lineH;
    int maxChars;
    int visibleLines;
};

ChatGeometry computeGeometry() {
    ChatGeometry g;
    g.outputTop = BORDER_PAD_Y + FM * LH + kTopGap;
    g.footerY = tftHeight - kBottomMargin - kFooterH;
    g.inputBarY = g.footerY - kInputBarH;
    g.sepY = g.inputBarY - kSepGap;
    g.lineH = FP * LH;
    g.maxChars = (tftWidth - 2 * BORDER_PAD_X) / (FP * LW);
    if (g.maxChars < 1) g.maxChars = 1;
    g.visibleLines = (g.sepY - g.outputTop) / g.lineH;
    if (g.visibleLines < 1) g.visibleLines = 1;
    return g;
}

// Greedy word-wrap of a single line (no embedded '\n'). Always emits at
// least one (possibly empty) line, so blank paragraphs still take up a row.
std::vector<String> wrapLine(const String &text, int maxChars) {
    std::vector<String> out;
    int n = text.length();
    int start = 0;
    String currentLine;
    while (start < n) {
        int spaceIdx = text.indexOf(' ', start);
        int wordEnd = (spaceIdx == -1) ? n : spaceIdx;
        String word = text.substring(start, wordEnd);
        while ((int)word.length() > maxChars) {
            if (currentLine.length() > 0) {
                out.push_back(currentLine);
                currentLine = "";
            }
            out.push_back(word.substring(0, maxChars));
            word = word.substring(maxChars);
        }
        if (currentLine.length() == 0) currentLine = word;
        else if ((int)(currentLine.length() + 1 + word.length()) <= maxChars) currentLine += " " + word;
        else {
            out.push_back(currentLine);
            currentLine = word;
        }
        start = (spaceIdx == -1) ? n : spaceIdx + 1;
    }
    out.push_back(currentLine);
    return out;
}

// Splits on explicit '\n' first (the model can emit these), then word-wraps
// each resulting segment independently.
std::vector<String> wrapText(const String &text, int maxChars) {
    std::vector<String> out;
    int start = 0;
    int n = text.length();
    for (;;) {
        int nl = text.indexOf('\n', start);
        String segment = (nl == -1) ? text.substring(start) : text.substring(start, nl);
        for (auto &l : wrapLine(segment, maxChars)) out.push_back(l);
        if (nl == -1) break;
        start = nl + 1;
    }
    return out;
}

struct ChatLine {
    String text;
    bool isUser;
};

struct ChatSession {
    // Each entry is one message ("You: ..." or the assistant's reply text,
    // appended to as tokens stream in). true = user message.
    std::vector<std::pair<bool, String>> messages;
    int scrollOffset = 0;
    bool followBottom = true;
    String inputBarPreview;
    BruceLMSettings settings;
};

std::vector<ChatLine> buildLines(const ChatSession &s, int maxChars) {
    std::vector<ChatLine> out;
    for (size_t i = 0; i < s.messages.size(); i++) {
        bool isUser = s.messages[i].first;
        for (auto &l : wrapText(s.messages[i].second, maxChars)) out.push_back({l, isUser});
        if (i + 1 < s.messages.size()) out.push_back({"", false}); // blank line between messages
    }
    return out;
}

// Applies rotary-encoder scroll input. Returns true if the visible content
// actually needs to move as a result.
bool handleScrollInput(ChatSession &s, int totalLines, int visibleLines) {
    int maxScroll = totalLines > visibleLines ? totalLines - visibleLines : 0;
    bool changed = false;
    if (check(NextPress)) { // rotate toward newer/bottom content
        if (s.scrollOffset < maxScroll) {
            s.scrollOffset++;
            changed = true;
        }
        s.followBottom = (s.scrollOffset >= maxScroll);
    }
    if (check(PrevPress)) { // rotate toward older/top content
        if (s.scrollOffset > 0) {
            s.scrollOffset--;
            changed = true;
        }
        s.followBottom = false;
    }
    return changed;
}

void clampScroll(ChatSession &s, int totalLines, int visibleLines) {
    int maxScroll = totalLines > visibleLines ? totalLines - visibleLines : 0;
    if (s.followBottom) s.scrollOffset = maxScroll;
    if (s.scrollOffset > maxScroll) s.scrollOffset = maxScroll;
    if (s.scrollOffset < 0) s.scrollOffset = 0;
}

String truncateToWidth(const String &text, int maxChars) {
    if ((int)text.length() <= maxChars) return text;
    if (maxChars <= 3) return text.substring(0, maxChars);
    return text.substring(0, maxChars - 3) + "...";
}

void drawChatFrame(const ChatGeometry &g, const String &inputBarPreview) {
    drawMainBorderWithTitle("BruceLM");

    tft.drawFastHLine(BORDER_PAD_X, g.sepY, tftWidth - 2 * BORDER_PAD_X, bruceConfig.priColor);

    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    int inputTextY = g.inputBarY + (kInputBarH - g.lineH) / 2;
    tft.setCursor(BORDER_PAD_X, inputTextY);
    String preview = inputBarPreview.length() ? ("> " + inputBarPreview) : "> Tap OK to type a prompt";
    tft.print(truncateToWidth(preview, g.maxChars));

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    String hint = "OK: new prompt  Esc: exit";
    int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
    int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
    if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
    tft.setCursor(hx, hintTextY);
    tft.print(hint);
}

void drawVisibleLines(const std::vector<ChatLine> &lines, int scrollOffset, const ChatGeometry &g) {
    for (int i = 0; i < g.visibleLines; i++) {
        int idx = scrollOffset + i;
        if (idx >= (int)lines.size()) break;
        tft.setTextColor(
            lines[idx].isUser ? bruceConfig.priColor : bruceConfig.secColor, bruceConfig.bgColor
        );
        tft.setCursor(BORDER_PAD_X, g.outputTop + i * g.lineH);
        tft.print(lines[idx].text);
    }
}

// Single entry point for both drawing and encoder-scroll handling, so the
// idle loop and the token-streaming loop can never disagree about how
// scrolling behaves. Returns true if it actually redrew the screen.
bool renderChat(ChatSession &s, bool force) {
    ChatGeometry g = computeGeometry();
    std::vector<ChatLine> lines = buildLines(s, g.maxChars);
    int total = (int)lines.size();

    bool scrolled = handleScrollInput(s, total, g.visibleLines);
    clampScroll(s, total, g.visibleLines);

    if (!force && !scrolled) return false;

    drawChatFrame(g, s.inputBarPreview);
    drawVisibleLines(lines, s.scrollOffset, g);
    return true;
}

void ensureFolders(FS &fs) {
    // sd_functions.cpp's createFolder() is the interactive file-manager
    // "New Folder" action - it always pops a keyboard prompt for a name and
    // creates it *under* the given path. We want these two fixed paths to
    // just silently exist, so call fs.mkdir() directly instead. Without an
    // SD card (LittleFS fallback, where these folders don't already exist
    // from a prior run) createFolder() here was popping the folder-name
    // keyboard twice on every launch before ever reaching the start screen.
    if (!folderExists(fs, kRootDir)) fs.mkdir(kRootDir);
    if (!folderExists(fs, kModelsDir)) fs.mkdir(kModelsDir);
}

// Draws a message starting at the top of the content area, an optional
// standout note centered in the gap between that text and the footer (for
// calling out the required action - e.g. "Press OK to..." - so it isn't
// just a small line buried in the bottom hint), and a hint row pinned to
// the bottom (same geometry the chat frame uses) so button mappings always
// land in the same place instead of wherever the message text happened to end.
void drawMessageScreen(const String &msg, const String &hint, const String &centerNote = "") {
    ChatGeometry g = computeGeometry();
    drawMainBorderWithTitle("BruceLM");

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int y = g.outputTop;
    for (auto &line : wrapText(msg, g.maxChars)) {
        tft.setCursor(BORDER_PAD_X, y);
        tft.print(line);
        y += g.lineH;
    }

    if (centerNote.length() > 0) {
        int aboveFooterY = g.footerY - 4;
        // Biased toward the footer (rather than dead-centered in the gap) so
        // the note sits a bit further down the screen.
        int gapSpace = max(0, aboveFooterY - y - g.lineH);
        int noteY = y + (gapSpace * 2) / 3;
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        for (auto &line : wrapText(centerNote, g.maxChars)) {
            int nx = (tftWidth - (int)(line.length() * FP * LW)) / 2;
            if (nx < BORDER_PAD_X) nx = BORDER_PAD_X;
            tft.setCursor(nx, noteY);
            tft.print(line);
            noteY += g.lineH;
        }
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
    int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
    if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
    tft.setCursor(hx, hintTextY);
    tft.print(hint);
}

void showMessageAndWait(const String &msg) {
    drawMessageScreen(msg, "Esc: return");
    while (!check(EscPress)) delay(30);
}

// Blocks until the user picks Select (true) or Esc (false).
bool showConfirm(const String &msg, const String &hint, const String &centerNote = "") {
    drawMessageScreen(msg, hint, centerNote);
    for (;;) {
        if (check(SelPress)) return true;
        if (check(EscPress)) return false;
        delay(30);
    }
}

float adjustFloat(float v, float step, float lo, float hi, int dir) {
    v += step * dir;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

int adjustInt(int v, int step, int lo, int hi, int dir) {
    v += step * dir;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

int cycleIndex(int idx, int count, int dir) { return (idx + dir + count) % count; }

constexpr int kSettingsRowCount = 7;
constexpr int kSettingsSaveRow = kSettingsRowCount;
constexpr int kSettingsNumItems = kSettingsRowCount + 1;

void ensureListScroll(int cursor, int itemCount, int visibleRows, int &scroll) {
    if (itemCount <= visibleRows) {
        scroll = 0;
        return;
    }
    if (cursor < scroll) scroll = cursor;
    if (cursor >= scroll + visibleRows) scroll = cursor - visibleRows + 1;
}

struct SettingsEditState {
    bool editing = false;
    int editRow = -1;
    float tempBackup = 0, topPBackup = 0, repPenBackup = 0;
    int maxTokBackup = 0, seedIdxBackup = 0;
};

void renderSettingsRows(
    const BruceLMSettings &s, int cursor, const SettingsEditState &edit, int startY, int rowH,
    int scrollOffset, int visibleRows
) {
    struct Row {
        const char *label;
        String value;
    } rows[kSettingsRowCount] = {
        {"Temperature", String(s.temperature, 1)},
        {"Top-p", String(s.topP, 2)},
        {"Rep. Penalty", String(s.repetitionPenalty, 2)},
        {"Max Tokens", String(s.maxTokens)},
        {"Seed", String(kSeedPresets[s.seedPresetIndex].label)},
        {"Chat Template", s.chatTemplateEnabled ? "On >" : "Off >"},
        {"Reset to Default", "Reset >"},
    };

    tft.setTextSize(FP);
    for (int i = 0; i < visibleRows; i++) {
        int idx = scrollOffset + i;
        int rowY = startY + i * rowH;
        tft.fillRect(BORDER_PAD_X, rowY, tftWidth - 2 * BORDER_PAD_X, rowH, bruceConfig.bgColor);
        if (idx >= kSettingsNumItems) continue;

        bool selected = (cursor == idx);

        if (idx == kSettingsSaveRow) {
            tft.setTextColor(selected ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
            String saveLabel = "[ Save ]";
            int sx = (tftWidth - (int)(saveLabel.length() * FP * LW)) / 2;
            tft.setCursor(sx, rowY + 2);
            tft.print(saveLabel);
            continue;
        }

        bool editingThis = (edit.editing && edit.editRow == idx);

        tft.setTextColor(selected ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(BORDER_PAD_X, rowY + 2);
        tft.print(rows[idx].label);

        String valueText = editingThis ? ("[ " + rows[idx].value + " ]") : rows[idx].value;
        int vx = tftWidth - BORDER_PAD_X - (int)(valueText.length() * FP * LW);
        tft.setCursor(vx, rowY + 2);
        tft.print(valueText);
    }
}

// Sub-menu opened from the "Chat Template" row: toggles the template on/off
// and lets the user re-type the tags with the same keyboard() overlay used
// for prompt entry. Mutates `s` in place - the parent settings screen owns
// persisting it (via its own Save row / Esc-to-save), so nothing is written
// to disk here.
void showChatTemplateScreen(BruceLMSettings &s) {
    constexpr int kNumRows = 3;
    constexpr int kSaveRow = 3; // bottom action row, same spot/style as the parent screen's Save
    constexpr int kNumItems = 4;
    int cursor = 0;
    int scroll = 0;
    ChatGeometry g = computeGeometry();
    int rowH = g.lineH + 4;
    int startY = g.outputTop;
    int visibleRows = max(1, (g.footerY - startY) / rowH);
    bool redraw = true;

    for (;;) {
        if (redraw) {
            drawMainBorderWithTitle("Chat Template");
            ensureListScroll(cursor, kNumItems, visibleRows, scroll);

            struct Row {
                const char *label;
                String value;
            } rows[kNumRows] = {
                {"Enabled",  s.chatTemplateEnabled ? "On" : "Off"},
                {"User tag", s.userTag                           },
                {"Bot tag",  s.botTag                            },
            };

            tft.setTextSize(FP);
            for (int i = 0; i < visibleRows; i++) {
                int idx = scroll + i;
                int rowY = startY + i * rowH;
                tft.fillRect(BORDER_PAD_X, rowY, tftWidth - 2 * BORDER_PAD_X, rowH, bruceConfig.bgColor);
                if (idx >= kNumItems) continue;

                bool selected = (cursor == idx);
                if (idx == kSaveRow) {
                    tft.setTextColor(selected ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
                    String saveLabel = "[ Save ]";
                    int sx = (tftWidth - (int)(saveLabel.length() * FP * LW)) / 2;
                    tft.setCursor(sx, rowY + 2);
                    tft.print(saveLabel);
                    continue;
                }

                tft.setTextColor(selected ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
                tft.setCursor(BORDER_PAD_X, rowY + 2);
                tft.print(rows[idx].label);

                int labelW = (int)String(rows[idx].label).length() + 1;
                String valueText = truncateToWidth(rows[idx].value, max(1, g.maxChars - labelW));
                int vx = tftWidth - BORDER_PAD_X - (int)(valueText.length() * FP * LW);
                tft.setCursor(vx, rowY + 2);
                tft.print(valueText);
            }

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String hint = "OK: edit  Esc: save+exit";
            int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
            tft.fillRect(
                BORDER_PAD_X, hintTextY - 2, tftWidth - 2 * BORDER_PAD_X, g.lineH + 4, bruceConfig.bgColor
            );
            int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
            if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
            tft.setCursor(hx, hintTextY);
            tft.print(hint);
            redraw = false;
        }

        // Every field here mutates `s` immediately (no separate edit-mode
        // buffer to revert) - Save and Esc are therefore equivalent, both
        // just returning to the parent screen, which owns actually
        // persisting `s` to disk. Kept as two controls anyway to match the
        // parent settings screen's look and feel.
        if (check(EscPress)) return;
        if (check(SelPress)) {
            if (cursor == kSaveRow) {
                return;
            } else if (cursor == 0) {
                s.chatTemplateEnabled = !s.chatTemplateEnabled;
            } else if (cursor == 1) {
                String v = keyboard(s.userTag, 32, "User tag:");
                if (v.length() > 0) s.userTag = v;
            } else {
                String v = keyboard(s.botTag, 32, "Bot tag:");
                if (v.length() > 0) s.botTag = v;
            }
            redraw = true;
        } else if (check(NextPress)) {
            cursor = (cursor + 1) % kNumItems;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + kNumItems - 1) % kNumItems;
            redraw = true;
        }

        delay(30);
    }
}

// Sub-menu opened from the "Reset to Default" row: explains what's about to
// happen, then a single centered "Confirm" action (same highlighted-row look
// as showStartScreen's Start/Settings rows) rather than a bare Select/Esc
// dialog, so it can't be triggered by one accidental button press. Returns
// true if the user confirmed (config file deleted); the caller is
// responsible for treating that as "abandon whatever was in progress".
bool showResetToDefaultScreen(FS &fs) {
    ChatGeometry g = computeGeometry();
    String msg = "Reset all BruceLM settings to\n"
                 "default values? This deletes\n"
                 "the saved config file.\n"
                 "\n"
                 "Models on the SD card are not\n"
                 "affected.";
    std::vector<String> lines = wrapText(msg, g.maxChars);

    int rowH = g.lineH + 6;
    int afterTextY = g.outputTop + (int)lines.size() * g.lineH + 4;
    int aboveFooterY = g.footerY - 4;
    int confirmY = afterTextY + max(0, (aboveFooterY - afterTextY - rowH) / 2);

    bool redraw = true;
    for (;;) {
        if (redraw) {
            drawMainBorderWithTitle("Reset to Default");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            int y = g.outputTop;
            for (auto &line : lines) {
                tft.setCursor(BORDER_PAD_X, y);
                tft.print(line);
                y += g.lineH;
            }

            tft.fillRect(BORDER_PAD_X, confirmY, tftWidth - 2 * BORDER_PAD_X, rowH, bruceConfig.bgColor);
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            String label = "> Confirm";
            int lx = (tftWidth - (int)(label.length() * FP * LW)) / 2;
            tft.setCursor(lx, confirmY + 2);
            tft.print(label);

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String hint = "OK: confirm  Esc: cancel";
            int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
            int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
            if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
            tft.setCursor(hx, hintTextY);
            tft.print(hint);
            redraw = false;
        }

        if (check(EscPress)) return false;
        if (check(SelPress)) {
            if (fs.exists(kConfigPath)) fs.remove(kConfigPath);
            return true;
        }
        delay(30);
    }
}

// ble_spam.cpp-style row editor: Next/Prev moves the cursor between rows,
// Select enters/exits edit mode on the highlighted row, Next/Prev while
// editing adjusts that row's value, Esc while editing reverts to the value
// it had before editing started, Esc while browsing saves and exits. The
// "Chat Template" and "Reset to Default" rows are the exceptions - Select on
// either always opens its own sub-screen instead of inline-editing, since
// neither is a single adjustable number.
//
// Returns true if the user reset to defaults (config file deleted) - the
// caller should treat this as "abandon whatever was in progress and go back
// to the top-level BruceLM menu" rather than resuming with the just-deleted
// settings.
bool showSettingsScreen(FS &fs) {
    BruceLMSettings s = loadSettings(fs);
    SettingsEditState edit;
    int cursor = 0;
    int scroll = 0;
    ChatGeometry g = computeGeometry();
    int rowH = g.lineH + 4;
    int startY = g.outputTop;
    int visibleRows = max(1, (g.footerY - startY) / rowH);

    bool redraw = true;
    drawMainBorderWithTitle("BruceLM Settings");

    constexpr int kChatTemplateRow = 5;          // opens showChatTemplateScreen() on Select
    constexpr int kResetRow = 6;                 // opens showResetToDefaultScreen() on Select
    constexpr int kSaveRow = kSettingsSaveRow;   // bottom action row, ble_spam "[ Start ]"-style
    constexpr int kNumItems = kSettingsNumItems; // 7 rows + save

    for (;;) {
        if (redraw) {
            ensureListScroll(cursor, kNumItems, visibleRows, scroll);
            renderSettingsRows(s, cursor, edit, startY, rowH, scroll, visibleRows);

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String hint = edit.editing ? "OK: confirm  Esc: revert" : "OK: edit  Esc: save+exit";
            int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
            tft.fillRect(
                BORDER_PAD_X, hintTextY - 2, tftWidth - 2 * BORDER_PAD_X, g.lineH + 4, bruceConfig.bgColor
            );
            int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
            if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
            tft.setCursor(hx, hintTextY);
            tft.print(hint);
            redraw = false;
        }

        if (check(EscPress)) {
            // Esc always means "save and exit" from the row list (or revert
            // while editing) - unchanged regardless of the Save row below.
            if (edit.editing) {
                switch (edit.editRow) {
                    case 0: s.temperature = edit.tempBackup; break;
                    case 1: s.topP = edit.topPBackup; break;
                    case 2: s.repetitionPenalty = edit.repPenBackup; break;
                    case 3: s.maxTokens = edit.maxTokBackup; break;
                    case 4: s.seedPresetIndex = edit.seedIdxBackup; break;
                }
                edit.editing = false;
                redraw = true;
            } else {
                saveSettings(fs, s);
                return false;
            }
        } else if (check(SelPress)) {
            if (edit.editing) {
                edit.editing = false;
            } else if (cursor == kSaveRow) {
                saveSettings(fs, s);
                return false;
            } else if (cursor == kResetRow) {
                if (showResetToDefaultScreen(fs)) return true;
                drawMainBorderWithTitle("BruceLM Settings");
            } else if (cursor == kChatTemplateRow) {
                showChatTemplateScreen(s);
                drawMainBorderWithTitle("BruceLM Settings");
            } else {
                edit.editing = true;
                edit.editRow = cursor;
                edit.tempBackup = s.temperature;
                edit.topPBackup = s.topP;
                edit.repPenBackup = s.repetitionPenalty;
                edit.maxTokBackup = s.maxTokens;
                edit.seedIdxBackup = s.seedPresetIndex;
            }
            redraw = true;
        } else if (edit.editing) {
            int dir = check(NextPress) ? 1 : (check(PrevPress) ? -1 : 0);
            if (dir != 0) {
                switch (edit.editRow) {
                    case 0: s.temperature = adjustFloat(s.temperature, 0.1f, 0.0f, 2.0f, dir); break;
                    case 1: s.topP = adjustFloat(s.topP, 0.05f, 0.0f, 1.0f, dir); break;
                    case 2:
                        s.repetitionPenalty = adjustFloat(s.repetitionPenalty, 0.05f, 1.0f, 2.0f, dir);
                        break;
                    case 3: s.maxTokens = adjustInt(s.maxTokens, 16, 16, 2048, dir); break;
                    case 4: s.seedPresetIndex = cycleIndex(s.seedPresetIndex, kNumSeedPresets, dir); break;
                }
                redraw = true;
            }
        } else if (check(NextPress)) {
            cursor = (cursor + 1) % kNumItems;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + kNumItems - 1) % kNumItems;
            redraw = true;
        }

        delay(30);
    }
}

enum class StartAction { Start, HowToRun, Settings, Exit };

// First screen: a one-line pitch above three selectable action rows
// ("Start" / "How to Run" / "Settings"), same cursor+Next/Prev+Select style
// as the settings screen above, with the button hints pinned to the bottom.
StartAction showStartScreen() {
    int cursor = 0;
    const char *labels[3] = {"Start", "How to Run", "Settings"};
    constexpr int kNumRows = 3;
    ChatGeometry g = computeGeometry();
    std::vector<String> pitchLines = wrapText("Run small LLMs locally on your Bruce device!", g.maxChars);

    int rowH = g.lineH + 6;
    // A fixed-size gap under the header for the pitch text, so the button
    // block's position doesn't shift around based on how long the pitch is.
    int headerGap = g.lineH * 3;
    int afterTextY = g.outputTop + headerGap;
    int aboveFooterY = g.footerY - 4;
    int rowsBlockH = rowH * kNumRows;
    // Biased toward the footer (rather than dead-centered in the gap) so the
    // buttons sit a bit further down the screen.
    int gapSpace = max(0, aboveFooterY - afterTextY - rowsBlockH);
    int firstRowY = afterTextY + (gapSpace * 2) / 3;
    // Pitch text biased toward the bottom of its header gap (rather than
    // dead-centered) so it sits a bit further down, closer to the Start button.
    int pitchBlockH = (int)pitchLines.size() * g.lineH;
    int pitchY = g.outputTop + max(0, ((headerGap - pitchBlockH) * 2) / 3);

    bool redraw = true;
    for (;;) {
        if (redraw) {
            drawMainBorderWithTitle("BruceLM");
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            int y = pitchY;
            for (auto &line : pitchLines) {
                int lx = (tftWidth - (int)(line.length() * FP * LW)) / 2;
                if (lx < BORDER_PAD_X) lx = BORDER_PAD_X;
                tft.setCursor(lx, y);
                tft.print(line);
                y += g.lineH;
            }

            for (int i = 0; i < kNumRows; i++) {
                int rowY = firstRowY + i * rowH;
                tft.fillRect(BORDER_PAD_X, rowY, tftWidth - 2 * BORDER_PAD_X, rowH, bruceConfig.bgColor);
                tft.setTextColor(cursor == i ? TFT_YELLOW : bruceConfig.priColor, bruceConfig.bgColor);
                String label = String("> ") + labels[i];
                int lx = (tftWidth - (int)(label.length() * FP * LW)) / 2;
                tft.setCursor(lx, rowY + 2);
                tft.print(label);
            }

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String hint = "OK: select  Esc: exit";
            int hintTextY = g.footerY + (kFooterH - g.lineH) / 2;
            int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
            if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
            tft.setCursor(hx, hintTextY);
            tft.print(hint);
            redraw = false;
        }

        if (check(EscPress)) return StartAction::Exit;
        if (check(SelPress)) {
            if (cursor == 0) return StartAction::Start;
            if (cursor == 1) return StartAction::HowToRun;
            return StartAction::Settings;
        }
        if (check(NextPress)) {
            cursor = (cursor + 1) % kNumRows;
            redraw = true;
        } else if (check(PrevPress)) {
            cursor = (cursor + kNumRows - 1) % kNumRows;
            redraw = true;
        }
        delay(30);
    }
}

// Opened from the "How to Run" row: where to get models and where to put
// them, ending in a single centered "Okay" action (same look as
// showResetToDefaultScreen's "Confirm") that just returns to the caller,
// which redisplays the start screen. The "Okay" row is pinned just above the
// footer hint regardless of content length (sized to just fit its own text,
// not a full row like the settings list), and the text above it scrolls with
// the encoder (same NextPress/PrevPress convention as the chat view) - on
// smaller screens the full text otherwise overflows past the border and
// overlaps the fixed-position button/hint row. When there's more content
// than fits, the last visible line is deliberately clipped to half height
// (rather than simply omitted) as a visual cue that there's more below.
void showHowToRunScreen() {
    ChatGeometry g = computeGeometry();
    // Built as explicit (text, indented, dimmed) rows rather than a single
    // wrapped string - wrapLine() treats runs of leading spaces as word
    // breaks and silently eats them, so indentation can't survive a plain
    // "\n"-joined string round-tripped through wrapText().
    struct InfoRow {
        String text;
        bool indented;
        bool dimmed;
    };
    std::vector<InfoRow> lines = {
        {"1. Download zipped model and",        false, false},
        {"tokenizer files from:",               true,  false},
        {"https://archive.org/details/BruceLM", true,  true },
        {"2. Unzip both files to SD card",      false, false},
        {"BruceLM/models/:",                    true,  false},
        {"models/Chat3.3M/chat3.3M.bin",        true,  true },
        {"models/Chat3.3M/tok1024.bin",         true,  true },
        {"models/Stories260K/stories260K.bin",  true,  true },
        {"models/Stories260K/tok512.bin",       true,  true },
        {"3. Turn off chat templates",          false, false},
        {"for stories260K",                     true,  false},
        {"4. You can create your own models",   false, false},
        {"using llama2.c by Karpathy",          true,  false},
    };
    int indentPx = FP * LW * 3;
    int lineH = g.lineH;

    int okayRowH = lineH + 4;             // just tall enough for the label, not a full list row
    int okayY = g.footerY - 4 - okayRowH; // pinned above the footer, independent of content length

    // Reserve one row at the top for the scroll-direction indicators, kept
    // out of the text flow itself.
    int textTop = g.outputTop;
    int contentBottom = okayY - 4;
    // Always leave room for a half-height clipped line at the bottom, so a
    // full line's worth of space isn't silently reserved-but-unused when
    // there's nothing left to scroll to.
    int visibleFullLines = max(1, (contentBottom - textTop - lineH / 2) / lineH);
    int scrollOffset = 0;
    int maxScroll = max(0, (int)lines.size() - visibleFullLines);

    bool redraw = true;
    for (;;) {
        if (redraw) {
            drawMainBorderWithTitle("How to Run");

            int y = textTop;
            int i = 0;
            for (; i < visibleFullLines; i++) {
                int idx = scrollOffset + i;
                if (idx >= (int)lines.size()) break;
                InfoRow &row = lines[idx];
                tft.setTextColor(
                    row.dimmed ? bruceConfig.secColor : bruceConfig.priColor, bruceConfig.bgColor
                );
                tft.setCursor(BORDER_PAD_X + (row.indented ? indentPx : 0), y);
                tft.print(row.text);
                y += lineH;
            }
            // One more line, then mask its bottom half back out with the
            // background color - Bruce's tft wrapper doesn't expose
            // TFT_eSPI's setViewport/clipping, so "print then paint over"
            // is the only way to fake a half-height clip here. Visual cue
            // that there's more content below to scroll to.
            int cutIdx = scrollOffset + i;
            if (cutIdx < (int)lines.size()) {
                InfoRow &row = lines[cutIdx];
                tft.setTextColor(
                    row.dimmed ? bruceConfig.secColor : bruceConfig.priColor, bruceConfig.bgColor
                );
                tft.setCursor(BORDER_PAD_X + (row.indented ? indentPx : 0), y);
                tft.print(row.text);
                tft.fillRect(
                    BORDER_PAD_X,
                    y + lineH / 2,
                    tftWidth - 2 * BORDER_PAD_X,
                    lineH - lineH / 2,
                    bruceConfig.bgColor
                );
            }

            tft.fillRect(BORDER_PAD_X, okayY, tftWidth - 2 * BORDER_PAD_X, okayRowH, bruceConfig.bgColor);
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            String label = "> Okay";
            int lx = (tftWidth - (int)(label.length() * FP * LW)) / 2;
            tft.setCursor(lx, okayY + 2);
            tft.print(label);

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String hint = "OK: back to menu  Esc: back";
            int hintTextY = g.footerY + (kFooterH - lineH) / 2;
            int hx = (tftWidth - (int)(hint.length() * FP * LW)) / 2;
            if (hx < BORDER_PAD_X) hx = BORDER_PAD_X;
            tft.setCursor(hx, hintTextY);
            tft.print(hint);
            redraw = false;
        }

        if (check(EscPress) || check(SelPress)) return;
        if (check(NextPress)) {
            if (scrollOffset < maxScroll) {
                scrollOffset++;
                redraw = true;
            }
        } else if (check(PrevPress)) {
            if (scrollOffset > 0) {
                scrollOffset--;
                redraw = true;
            }
        }
        delay(30);
    }
}

void showLoadingScreen(const String &modelFileName, const String &tokenizerFileName) {
    ChatGeometry g = computeGeometry();
    drawMainBorderWithTitle("BruceLM");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(BORDER_PAD_X, g.outputTop);
    tft.print("Loading model + tokenizer...");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(BORDER_PAD_X, g.outputTop + g.lineH * 2);
    tft.print(truncateToWidth(modelFileName, g.maxChars));
    tft.setCursor(BORDER_PAD_X, g.outputTop + g.lineH * 3);
    tft.print(truncateToWidth(tokenizerFileName, g.maxChars));
}

// Streams the transcript into the top output area; returns false if the user
// cancelled generation (Esc) partway through.
bool runChatTurn(ChatSession &session, LLMEngine &engine, const String &userPrompt) {
    session.messages.push_back({true, "You: " + userPrompt});
    session.messages.push_back({false, "> "}); // "> " prefix, same style as the input bar preview
    session.inputBarPreview = userPrompt;
    session.followBottom = true;
    renderChat(session, /*force=*/true);

    GenerationParams params;
    params.temperature = session.settings.temperature;
    params.topP = session.settings.topP;
    params.repetitionPenalty = session.settings.repetitionPenalty;
    params.seed = session.settings.seed();
    params.chatTemplateEnabled = session.settings.chatTemplateEnabled;
    params.userTag = session.settings.userTag;
    params.botTag = session.settings.botTag;

    bool cancelled = false;
    engine.generate(userPrompt, session.settings.maxTokens, params, [&](const String &piece) -> bool {
        // Generation can run longer than Bruce's normal screen-timeout
        // window. wakeUpScreen() just resets its inactivity clock (the
        // same thing any real button press does) - calling it here from
        // our own module, not touching core/display.cpp itself, keeps
        // the screen from dimming/sleeping mid-reply.
        wakeUpScreen();
        if (check(EscPress)) {
            cancelled = true;
            return false;
        }
        session.messages.back().second += piece;
        renderChat(session, /*force=*/true);
        return true;
    });

    if (cancelled) {
        session.messages.back().second += " [cancelled]";
        renderChat(session, /*force=*/true);
    }
    return !cancelled;
}

// Exit: user backed out of the chat (Esc) - bruceLM_setup() should exit the
// whole module. ResetToMenu: settings were reset to defaults from within the
// chat - bruceLM_setup() should abandon this session and redisplay the
// top-level start screen instead.
enum class ChatLoopResult { Exit, ResetToMenu };

ChatLoopResult chatLoop(LLMEngine &engine, FS &fs, const BruceLMSettings &settings) {
    ChatSession session;
    session.settings = settings;
    renderChat(session, /*force=*/true);

    for (;;) {
        if (check(EscPress)) return ChatLoopResult::Exit;
        if (check(SelPress)) {
            String prompt = keyboard("", 200, "Prompt:");
            String trimmed = prompt;
            trimmed.trim();
            trimmed.toLowerCase();
            if (trimmed == "/settings") {
                // No spare physical input to dedicate to "open settings" while
                // chatting (Next/Prev already scroll, Select starts a prompt,
                // Esc exits) - reusing the existing prompt entry as a command
                // avoids adding a new input mapping.
                if (showSettingsScreen(fs)) return ChatLoopResult::ResetToMenu;
                session.settings = loadSettings(fs);
                renderChat(session, /*force=*/true);
            } else if (prompt.length() > 0) {
                runChatTurn(session, engine, prompt);
            }
        } else {
            renderChat(session, /*force=*/false); // only redraws if the encoder scrolled
        }
        delay(30);
    }
}

// IncompatibleGroupSize and ConfigTooLarge are both "this will probably go
// badly, but try anyway?" situations - one confirm+retry path for both
// instead of duplicating the same dialog/retry logic per error code.
LLMLoadError confirmRiskyLoad(
    FS &fs, const String &checkpointPath, const String &tokenizerPath, LLMEngine &engine, LLMLoadError err,
    const String &modelFileName, const String &tokenizerFileName
) {
    String warning;
    if (err == LLMLoadError::IncompatibleGroupSize) {
        warning = "This model's quantization group\n"
                  "size doesn't evenly divide its\n"
                  "layer sizes - output will likely\n"
                  "come out garbled.";
    } else if (err == LLMLoadError::ConfigTooLarge) {
        warning = "This model looks too large for\n"
                  "available memory - it may fail to\n"
                  "load or crash mid-reply.";
    } else {
        return err;
    }

    if (!showConfirm(warning, "OK: run anyway   Esc: cancel")) return err;
    showLoadingScreen(modelFileName, tokenizerFileName);
    return engine.load(fs, checkpointPath, tokenizerPath, /*overrideSafetyChecks=*/true);
}

} // namespace

void bruceLM_setup() {
    FS *fs;
    if (!getFsStorage(fs)) {
        showMessageAndWait("No storage available (SD/LittleFS).");
        return;
    }

    ensureFolders(*fs);

    for (;;) {
        StartAction action = showStartScreen();
        if (action == StartAction::Exit) return;
        if (action == StartAction::HowToRun) {
            showHowToRunScreen();
            continue;
        }
        if (action == StartAction::Settings) {
            // Return value (reset-to-default) is irrelevant here - we're
            // already about to redisplay the start screen either way.
            showSettingsScreen(*fs);
            continue;
        }
        // action == StartAction::Start

        BruceLMSettings settings = loadSettings(*fs);

        if (!showConfirm(
                "Select a model checkpoint file.\n"
                "\n"
                "(e.g. \"chat3.3M.bin\")",
                "OK: continue   Esc: cancel",
                "Press OK to open the file picker"
            ))
            return;
        String checkpointPath = loopSD(*fs, true, "bin", kModelsDir);
        if (checkpointPath.length() == 0) return; // user backed out of the picker

        if (!showConfirm(
                "Select the tokenizer file that\n"
                "matches your model.\n"
                "\n"
                "(e.g. \"tok1024.bin\")",
                "OK: continue   Esc: cancel",
                "Press OK to open the file picker"
            ))
            return;
        String tokenizerPath = loopSD(*fs, true, "bin", kModelsDir);
        if (tokenizerPath.length() == 0) return; // user backed out of the picker

        String modelFileName = checkpointPath.substring(checkpointPath.lastIndexOf('/') + 1);
        String tokenizerFileName = tokenizerPath.substring(tokenizerPath.lastIndexOf('/') + 1);
        showLoadingScreen(modelFileName, tokenizerFileName);

        LLMEngine engine;
        LLMLoadError err = engine.load(*fs, checkpointPath, tokenizerPath);

        if (err == LLMLoadError::IncompatibleGroupSize || err == LLMLoadError::ConfigTooLarge)
            err = confirmRiskyLoad(
                *fs, checkpointPath, tokenizerPath, engine, err, modelFileName, tokenizerFileName
            );

        switch (err) {
            case LLMLoadError::None: break;
            case LLMLoadError::CheckpointNotFound:
            case LLMLoadError::TokenizerNotFound:
                showMessageAndWait("Model or tokenizer file went missing.");
                return;
            case LLMLoadError::BadMagicOrVersion:
                showMessageAndWait(
                    "Unrecognized checkpoint format.\n"
                    "Expected a llama2.c export.py v1/v2\n"
                    "file, or the original header-less\n"
                    "format karpathy/tinyllamas ships."
                );
                return;
            case LLMLoadError::ConfigTooLarge:
                showMessageAndWait("Could not load: model too large for available memory.");
                return;
            case LLMLoadError::OutOfMemory:
                showMessageAndWait("Out of memory while allocating model buffers.");
                return;
            case LLMLoadError::IncompatibleGroupSize:
                showMessageAndWait("Could not load: incompatible quantized model.");
                return;
        }

        // ResetToMenu (settings reset to default from within the chat)
        // loops back to the top-level start screen instead of exiting.
        if (chatLoop(engine, *fs, settings) == ChatLoopResult::ResetToMenu) continue;
        return;
    }
}
