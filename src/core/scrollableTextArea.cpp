#include "scrollableTextArea.h"
#define _scrollBuffer tft
ScrollableTextArea::ScrollableTextArea(const String &title)
    : firstVisibleLine{0}, _redraw{true}, _title(title), _fontSize(FP), _startX(BORDER_PAD_X),
      _startY(BORDER_PAD_Y), _width(tftWidth - 2 * BORDER_PAD_X),
      _height(tftHeight - BORDER_PAD_X - BORDER_PAD_Y) {
    drawMainBorder();

    if (!_title.isEmpty()) {
        printTitle(_title);
        _startY = tft.getCursorY();
        _height -= (_startY - BORDER_PAD_Y);
    }

    setup();
}

ScrollableTextArea::ScrollableTextArea(
    uint8_t fontSize, int16_t startX, int16_t startY, int32_t width, int32_t height, bool drawBorders,
    bool indentWrappedLines
)
    : firstVisibleLine{0}, _redraw{true}, _title(""), _fontSize(fontSize), _startX(startX), _startY(startY),
      _width(width), _height(height), _indentWrappedLines(indentWrappedLines) {
    if (drawBorders) { drawMainBorder(); }
    setup();
}

ScrollableTextArea::~ScrollableTextArea() {
    // We don't use Sprites for big things, unfortunetly theres no much RAM in all devices
}

void ScrollableTextArea::setup() {
    _scrollBuffer.setTextColor(bruceConfig.priColor);
    _scrollBuffer.setTextSize(_fontSize);
    _scrollBuffer.fillRect(_startX, _startY, _width, _height, bruceConfig.bgColor);

    _maxCharactersPerLine = floor(_width / _scrollBuffer.textWidth("w", _fontSize));
    _pixelsPerLine = _scrollBuffer.fontHeight() + 2;
    _maxVisibleLines = floor(_height / _pixelsPerLine);
}

void ScrollableTextArea::scrollUp() {
    if (firstVisibleLine) {
        firstVisibleLine--;
        _redraw = true;
    }
}

void ScrollableTextArea::scrollDown() {
    if (firstVisibleLine + _maxVisibleLines <= linesBuffer.size()) {
        if (firstVisibleLine == 0) firstVisibleLine++;
        firstVisibleLine++;
        _redraw = true;
    }
}

void ScrollableTextArea::scrollToLine(size_t lineNumber) {
    if (linesBuffer.empty()) return; // Ensure there's content to scroll

    if (lineNumber > linesBuffer.size() - _maxVisibleLines) {
        firstVisibleLine =
            (linesBuffer.size() > _maxVisibleLines) ? linesBuffer.size() - _maxVisibleLines : 0;
    } else {
        firstVisibleLine = lineNumber;
    }
}

String ScrollableTextArea::getLine(size_t lineNumber) {
    return linesBuffer[(lineNumber >= linesBuffer.size()) ? linesBuffer.size() : lineNumber];
}

size_t ScrollableTextArea::getMaxLines() { return linesBuffer.size(); }

void ScrollableTextArea::show(bool force) {
    draw(force);

    while (check(SelPress)) {
        update(force);
        yield();
    }
    while (!check(SelPress)) {
        update(force);
        yield();
    }
}

uint32_t ScrollableTextArea::getMaxVisibleTextLength() { return _maxVisibleLines * _maxCharactersPerLine; }

void ScrollableTextArea::update(bool force) {
    if (check(PrevPress) || check(UpPress)) scrollUp();
    else if (check(NextPress) || check(DownPress)) scrollDown();

    draw(force);
}

void ScrollableTextArea::fromFile(File file) {
    linesBuffer.clear();
    while (file.available()) addLine(file.readStringUntil('\n'));

    draw(true);
    delay(100);
    draw(true);
}

void ScrollableTextArea::clear() {
    firstVisibleLine = 0;
    linesBuffer.clear();
}

void ScrollableTextArea::fromString(const String &text) {
    clear();
    int startIdx = 0;
    int endIdx = 0;

    while (endIdx < text.length()) {
        if (text[endIdx] == '\n') {
            addLine(text.substring(startIdx, endIdx));
            startIdx = endIdx + 1;
        }

        endIdx++;
    }

    // Add the last line if there’s remaining text (text does not ends with \n)
    if (startIdx < text.length()) { addLine(text.substring(startIdx, endIdx)); }
}

// for devices it will act as a scrollable text area
void ScrollableTextArea::addLine(const String &text) {
    if (text.isEmpty()) {
        linesBuffer.emplace_back("");
        return;
    }

    String buff;
    size_t start = 0;
    bool firstLine = true;

    // Automatically split into multiple lines
    while (start < text.length()) {
        size_t len = _maxCharactersPerLine;

        if (!firstLine && _indentWrappedLines) {
            buff = " " + text.substring(start, start + len - 1); // Reduce length for space
            start += len - 1;
        } else {
            buff = text.substring(start, start + len);
            start += len;
        }
        if (buff.endsWith("\r")) buff.remove(buff.length() - 1);

        linesBuffer.emplace_back(buff);
        firstLine = false;
    }

    _redraw = true;
}

void ScrollableTextArea::draw(bool force) {
    if (!_redraw && !force) return;

    _scrollBuffer.fillRect(_startX, _startY, _width, _height, bruceConfig.bgColor);
    _scrollBuffer.setTextColor(bruceConfig.priColor);
    uint8_t _fSize = tft.getTextSize();
    tft.setTextSize(FP);

    uint16_t yOffset = 0;
    size_t lines = 0;

    // if there is text above
    if (firstVisibleLine) {
        _scrollBuffer.drawString("...", 0 + _startX, _startY + yOffset);
        yOffset += _pixelsPerLine;
        lines++;
    }

    int32_t tmpHeight = _height;
    // if there is text below
    if (linesBuffer.size() - firstVisibleLine >= _maxVisibleLines) {
        _scrollBuffer.drawString("...", 0 + _startX, _startY + _height - _pixelsPerLine);
        tmpHeight -= _pixelsPerLine;
        lines++;
    }

    size_t idx{firstVisibleLine};
    while (yOffset < tmpHeight && lines < _maxVisibleLines && idx < linesBuffer.size()) {
        _scrollBuffer.drawString(linesBuffer[idx], 0 + _startX, _startY + yOffset);
        yOffset += _pixelsPerLine;
        lines++;
        idx++;
    }

    lastVisibleLine = firstVisibleLine + lines;
    tft.setTextFont(_fSize);

    _redraw = false;
}
