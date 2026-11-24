#ifndef __BRUCE_I18N_H__
#define __BRUCE_I18N_H__

#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bruce_i18n {

enum class Language : uint8_t {
    EN = 0,
    ZH_CN = 1,
};

Language getLanguage();
bool isChinese();
void setLanguage(Language lang);
void setLanguage(const String &code);
const char *languageCode(Language lang);
String normalizeLanguageCode(const String &code);

// Translate an English UI string to the active firmware language.
// Returns the input unchanged when English is active or no entry exists.
const char *tr(const char *en);
String tr(const String &en);

// Pure UTF-8 and display-metric helpers (compatible with Bruce LW=6, LH=8 grid).
bool containsNonAscii(const char *s);
bool containsNonAscii(const String &s);
uint32_t utf8NextCodepoint(const char *&p);
bool isWideCodepoint(uint32_t cp);

// Pixel advance width of a single Unicode codepoint at Bruce `textSize`.
int16_t glyphAdvancePx(uint32_t cp, uint8_t textSize);

// Total pixel width of a UTF-8 string at Bruce `textSize`.
int16_t utf8PixelWidth(const char *s, uint8_t textSize);
int16_t utf8PixelWidth(const String &s, uint8_t textSize);

// Line height in pixels for a UTF-8 string at Bruce `textSize`.
int16_t utf8LineHeight(uint8_t textSize);

// Truncate UTF-8 string so its rendered width fits within `maxPx` pixels
// without splitting any multi-byte UTF-8 sequence.
String utf8TruncateToWidth(const String &s, int16_t maxPx, uint8_t textSize);

// Wrap UTF-8 string into lines that each fit within `maxPx` pixels.
std::vector<String> utf8WrapToWidth(const String &s, int16_t maxPx, uint8_t textSize);

// Return the largest byte length <= maxBytes that does not split a UTF-8 codepoint.
size_t utf8SafeCutBytes(const char *s, size_t len, size_t maxBytes);

} // namespace bruce_i18n

#endif // __BRUCE_I18N_H__
