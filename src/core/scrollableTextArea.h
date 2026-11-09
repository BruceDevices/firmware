#include "display.h"

class ScrollableTextArea {
public:
    ScrollableTextArea(const String &title = "");

    ScrollableTextArea(
        uint8_t fontSize, int16_t startX, int16_t startY, int32_t width, int32_t height,
        bool drawBorders = true, bool indentWrappedLines = false
    );

    ~ScrollableTextArea();

    void scrollUp();

    void scrollDown();

    void scrollToLine(size_t lineNumber);

    String getLine(size_t lineNumber);
    size_t getMaxLines();
    size_t getMaxVisibleLines() const { return _maxVisibleLines; }

    void addLine(const String &text);

    void clear();

    void fromString(const String &text);

    void fromFile(File file);

    void draw(bool force = false);

    void show(bool force = false);

    uint32_t getMaxVisibleTextLength();

    size_t firstVisibleLine;
    size_t lastVisibleLine;
    /// TODO: Change to std::vector<char *> and alloc to PSRAM if possible
    std::vector<String> linesBuffer;

private:
    bool _redraw;
    String _title;
    uint8_t _fontSize;
    int16_t _startX, _startY;
    int32_t _width, _height;
    int32_t _pixelsPerLine;
    size_t _maxVisibleLines;
    uint16_t _maxCharactersPerLine;
    bool _indentWrappedLines;

    void setup();

    void update(bool force = false);
};
