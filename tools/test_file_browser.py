"""Compile and exercise the production comparator, indexed source and page geometry."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(os.environ.get('BRUCE_FILE_TEST_ROOT', Path(__file__).resolve().parents[1]))
DISPLAY = Path(os.environ.get('BRUCE_FILE_TEST_DISPLAY', ROOT / 'src/core/display.cpp')).read_text(encoding='utf-8')
SD = Path(os.environ.get('BRUCE_FILE_TEST_SD', ROOT / 'src/core/sd_functions.cpp')).read_text(encoding='utf-8')
MENU = (ROOT / 'src/core/menu_items/FileMenu.cpp').read_text(encoding='utf-8')
HAS_SELECT_OUTPUT = 'bool *longPressResult' in DISPLAY

def definition(text, marker, semicolon=False):
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end + int(semicolon)]

production = '\n'.join([
    definition(DISPLAY, 'class MenuListSource {', True),
    r'''
static int lastType, lastIndex, lastPageSize;
static bool lastInterpreter, lastLetters, lastBorder, lastCompact;
static size_t lastSize;
static String lastTitle;
static bool *lastOutput;
template<typename... Output>
int loopMenuOptions(MenuListSource &source, uint8_t type, const char *title, int index,
                    bool interpreter, bool letters, uint16_t pageSize, bool border, bool compact,
                    Output... output) {
    lastSize = source.size(); lastType = type; lastTitle = title; lastIndex = index;
    lastInterpreter = interpreter; lastLetters = letters; lastPageSize = pageSize;
    lastBorder = border; lastCompact = compact;
    lastOutput = nullptr;
    ((lastOutput = output), ...);
    if (lastOutput) *lastOutput = true;
    return index;
}
''',
    definition(DISPLAY, 'struct MenuPage {', True),
    definition(DISPLAY, 'MenuPage calculateMenuPage('),
    definition(DISPLAY[DISPLAY.index('namespace {\nOpt_Coord drawMenuOptions('):], 'Opt_Coord drawMenuOptions('),
    definition(DISPLAY, 'int loopOptions(\n    std::vector<Option> &options,'),
    definition(DISPLAY, 'int loopFileOptions('),
    definition(SD, 'bool sortList('),
    definition(MENU, 'void FileMenu::optionsMenu()'),
])

PREAMBLE = r'''
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <new>
#include <string>
#include <vector>
struct String : std::string {
    using std::string::string;
    String(const std::string &s) : std::string(s) {}
    String substring(size_t from, size_t to) const { return substr(from, to - from); }
};
static size_t allocations = 0;
void *operator new(std::size_t n) { ++allocations; if (void *p = std::malloc(n)) return p; throw std::bad_alloc(); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
struct FileList { String filename; bool folder; bool operation; };
struct Option {
    String label;
    std::function<void()> operation;
    bool selected = false, enabled = true;
    bool (*hover)(void *, bool) = nullptr;
    void *hoverPointer = nullptr;
    bool hovered = false, hasColor = false;
    uint16_t color = 0, x = 0, y = 0, w = 0, h = 0;
    bool contain(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    Option(const String &s, std::function<void()> fn) : label(s), operation(fn) {}
};
struct { uint16_t priColor = 20, bgColor = 0, secColor = 30; } bruceConfig;
const uint16_t ALCOLOR = 40;
uint16_t getColorVariation(uint16_t color) { return color + 1; }
#define FM 1
#define LH 8
#define LW 6
#define BORDER_OFFSET_FROM_SCREEN_EDGE 5
#define TFT_DARKGREY 100
static int tftHeight = 100, tftWidth = 320;
struct Opt_Coord { uint16_t x = 0, y = 0, size = 10, fgcolor = 20, bgcolor = 0; };
struct FakeTft {
    int cursorY = 0;
    std::vector<String> printed;
    template<typename... T> void fillRoundRect(T...) {}
    template<typename... T> void drawRoundRect(T...) {}
    template<typename... T> void fillRect(T...) {}
    template<typename... T> void drawPixel(T...) {}
    template<typename... T> void setTextColor(T...) {}
    template<typename... T> void setTextSize(T...) {}
    template<typename... T> void drawCentreString(T...) {}
    template<typename... T> void drawString(T...) {}
    void setCursor(int, int y) { cursorY = y; }
    int getCursorY() const { return cursorY; }
    void println(const String &s) { printed.push_back(s); cursorY += LH * FM; }
} tft;
void TouchFooter() {}
#ifdef HAS_TOUCH
static bool s_pageUpVisible, s_pageDownVisible;
static int s_pageUpX, s_pageUpY, s_pageUpW, s_pageUpH, s_pageDownX, s_pageDownY, s_pageDownW, s_pageDownH;
static int s_pageUpTargetIndex, s_pageDownTargetIndex;
#endif
#define LITE_VERSION
#define MENU_TYPE_SUBMENU 1
#define MENU_TYPE_REGULAR 2
struct FakeFs {} SD, LittleFS;
static std::vector<Option> options;
static int mountCalls = 0, sdCalls = 0, littleCalls = 0, errorCalls = 0;
static bool mountResult = true;
bool setupSdCard() { ++mountCalls; return mountResult; }
void loopSD(FakeFs &fs) { if (&fs == &SD) ++sdCalls; else ++littleCalls; }
void displayError(const char *message, bool wait) { assert(std::string(message) == "Fail Mounting SD" && wait); ++errorCalls; }
void loopOptionsWebUi() {}
void loopOptions(std::vector<Option> &, int, const char *) {}
struct FileMenu { void optionsMenu(); void addOptionToMainMenu() {} };
'''

TESTS = r'''
struct ExpectedPage { int start, count; bool up, down; };
bool hoverProbe(void *context, bool render) { ++*static_cast<int *>(context); return render; }
int main() {
    // Existing page construction is the oracle, including tiny touch displays.
    size_t pageCases = 0;
    for (int rows = 1; rows <= 30; ++rows) for (int size = 1; size <= 300; ++size) {
        for (bool touch : {false, true}) {
            std::vector<ExpectedPage> pages;
            int maximum = 0;
            if (size > rows) {
                int remaining = size, start = 0;
                while (remaining > 0) {
                    bool up = touch && !pages.empty();
                    int capacity = std::max(1, rows - int(up));
                    int count = std::min(remaining, capacity);
                    bool down = touch && remaining > count;
                    if (down) {
                        int withDown = std::max(1, rows - int(up) - 1);
                        count = std::min(count, withDown);
                        if (count >= remaining) down = false;
                    }
                    pages.push_back({start, count, up, down});
                    maximum = std::max(maximum, count + int(up) + int(down));
                    start += count;
                    remaining -= count;
                }
            } else { pages.push_back({0, size, false, false}); maximum = size; }
            size_t p = 0;
            for (int index = 0; index < size; ++index) {
                if (index >= pages[p].start + pages[p].count) ++p;
                auto expected = pages[p];
                size_t before = allocations;
                auto page = calculateMenuPage(size, index, rows, touch);
                assert(allocations == before);
                assert(page.start == expected.start && page.count == expected.count && page.rows == maximum);
                assert(page.pageUp == expected.up && page.pageDown == expected.down);
                if (page.pageUp) assert(page.upTarget == pages[p-1].start);
                if (page.pageDown) assert(page.downTarget == pages[p+1].start);
                ++pageCases;
            }
        }
    }
    std::vector<FileList> files = {
        {"z-file-name-that-exceeds-small-string-storage.bin", false, false},
        {"Alpha-file-name-that-exceeds-small-string-storage", false, false},
        {"alpha-file-name-that-exceeds-small-string-storage-long", false, false},
        {"z-folder", true, false}, {"A-folder", true, false}, {"", false, false},
        {"\xC3\xA9-file", false, false}, {"\xC3\x89-file", false, false}
    };
    auto expected = files;
    std::sort(expected.begin(), expected.end(), [](const FileList &a, const FileList &b) {
        if (a.folder != b.folder) return a.folder > b.folder;
        auto left = a.filename, right = b.filename;
        for (char &c : left) c = std::toupper(static_cast<unsigned char>(c));
        for (char &c : right) c = std::toupper(static_cast<unsigned char>(c));
        return left < right;
    });
    size_t before = allocations;
    std::sort(files.begin(), files.end(), sortList);
    assert(allocations == before);
    for (size_t i = 0; i < files.size(); ++i) assert(files[i].filename == expected[i].filename);
    files.push_back({"> Back", false, true});
    before = allocations;
    MenuListSource source(files);
    assert(allocations == before && source.size() == files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        assert(&source.rawLabel(i) == &files[i].filename);
        assert(source.enabled(i) && !source.selected(i));
        source.markHovered(i); source.renderHover(i); source.run(i);
    }
    assert(allocations == before);
    assert(source.label(0) == "A-folder/" && source.hasColor(0));
    assert(source.color(0) == getColorVariation(bruceConfig.priColor));
    assert(source.label(files.size()-1) == "> Back" && source.color(files.size()-1) == ALCOLOR);
    source.clearBounds();
    source.setBounds(3, 5, 10, 100, 20); source.setBounds(4, 5, 30, 100, 20);
    assert(source.hitTest(5,10) == 3 && source.hitTest(104,49) == 4);
    assert(source.hitTest(105,30) == -1 && source.hitTest(5,50) == -1 && source.hitTest(5,9) == -1);
    source.clearBounds(); assert(source.hitTest(5,10) == -1);
    int calls = 0, hovers = 0;
    std::vector<Option> actions = {{"first", [&]{++calls;}}, {"disabled", []{}}};
    actions[1].enabled = false;
    actions[0].hover = hoverProbe; actions[0].hoverPointer = &hovers;
    MenuListSource normal(actions);
    normal.markHovered(1); assert(!actions[0].hovered && actions[1].hovered);
    normal.markHovered(0); assert(actions[0].hovered && !actions[1].hovered);
    assert(normal.renderHover(0) && hovers == 1 && !normal.renderHover(1));
    normal.run(0); assert(calls == 1 && !normal.enabled(1));
    normal.setBounds(0, 5,10,100,20); normal.setBounds(1,5,30,100,20);
    assert(normal.hitTest(5,10) == 0 && normal.hitTest(5,30) == -1);
    normal.clearBounds(); assert(actions[0].w == 0 && normal.hitTest(5,10) == -1);
#ifdef HAS_SELECT_OUTPUT
    bool longPress = false;
    assert(loopOptions(actions, 1, "actions", 1, true, false, 7, true, true, &longPress) == 1);
    assert(lastOutput == &longPress && longPress);
#else
    assert(loopOptions(actions, 1, "actions", 1, true, false, 7, true, true) == 1);
    assert(lastOutput == nullptr);
#endif
    assert(lastSize == actions.size() && lastType == 1 && lastTitle == "actions" && lastIndex == 1);
    assert(lastInterpreter && !lastLetters && lastPageSize == 7 && lastBorder && lastCompact);
#ifdef HAS_SELECT_OUTPUT
    longPress = false;
    assert(loopFileOptions(files, 2, 9, &longPress) == 2 && longPress && lastOutput == &longPress);
#else
    assert(loopFileOptions(files, 2, 9) == 2 && lastOutput == nullptr);
#endif
    assert(lastSize == files.size() && lastType == 2 && lastTitle.empty() && lastIndex == 2);
    assert(!lastInterpreter && lastLetters && lastPageSize == 9 && !lastBorder && !lastCompact);
    std::vector<Option> emptyActions;
    std::vector<FileList> emptyFiles;
    MenuListSource emptyNormal(emptyActions), emptyFile(emptyFiles);
    drawMenuOptions(0, emptyNormal, 20, 30, 0, true, false, false);
    drawMenuOptions(0, emptyFile, 20, 30, 0, true, false, false);
    assert(tft.printed.empty());
    std::vector<FileList> manyFiles;
    for (int i = 0; i < 10000; ++i) manyFiles.push_back({"entry-" + std::to_string(i), false, false});
    MenuListSource many(manyFiles);
    drawMenuOptions(9998, many, 20, 30, 0, true, false, false);
    assert(!tft.printed.empty() && tft.printed.size() <= 6);
    for (const auto &line : tft.printed) assert(line.find("entry-999") != std::string::npos);
    FileMenu menu; menu.optionsMenu(); assert(mountCalls == 0);
    options[1].operation(); assert(littleCalls == 1 && mountCalls == 0);
    mountResult = false; options[0].operation(); assert(mountCalls == 1 && sdCalls == 0 && errorCalls == 1);
    mountResult = true; options[0].operation(); assert(mountCalls == 2 && sdCalls == 1);
    std::cout << "PASS " << pageCases << " page cases; allocation-free sorting/source; labels/colors/touch/actions; lazy mount\n";
}
'''

compiler = shutil.which('g++')
if not compiler:
    raise SystemExit('BLOCKED: g++ is required for the host regression test')
with tempfile.TemporaryDirectory(prefix='bruce-file-browser-test-') as temporary:
    temporary = Path(temporary)
    source = temporary / 'test.cpp'
    binary = temporary / ('test.exe' if os.name == 'nt' else 'test')
    source.write_text(PREAMBLE + production + TESTS, encoding='utf-8')
    for variant in ([], ['-DHAS_TOUCH']):
        defines = ['-DHAS_SELECT_OUTPUT'] if HAS_SELECT_OUTPUT else []
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', *variant, *defines, str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
