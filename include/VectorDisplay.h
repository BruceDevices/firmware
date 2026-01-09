#ifndef __VECTOR_DISPLAY_H__
#define __VECTOR_DISPLAY_H__

#ifndef ARDUINO
#define NO_SERIAL
#include <stdint.h>
#define pgm_read_byte_near(a) (*(uint8_t *)(a))

#include <string>
#include <sys/timeb.h>

typedef std::string String;

uint32_t millis() {
    struct timeb t;
    ftime(&t);
    return t.millitm + t.time * 1000;
}

class Print {
public:
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *s, size_t length) {
        size_t wrote = 0;
        while (length > 0) {
            size_t b = write(*s++);
            if (b <= 0) break;
            b++;
            length--;
        }
        return wrote;
    }

    virtual size_t write(const char *s) { return write((uint8_t *)s, strlen(s)); }
};

class Stream : public Print {
public:
    virtual int read() = 0;
    virtual int available() = 0;
};

#else
#include <Arduino.h>
#include <SPI.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#define VECTOR_DISPLAY_MESSAGE_SIZE 8
#define VECTOR_DISPLAY_MAX_STRING 256

#define VECTOR_DISPLAY_DEFAULT_WIDTH 240
#define VECTOR_DISPLAY_DEFAULT_HEIGHT 320

#define ALIGN_LEFT 'l'
#define ALIGN_RIGHT 'r'
#define ALIGN_CENTER 'c'
#define ALIGN_TOP 't'
#define ALIGN_BOTTOM 'b'
#define ALIGN_BASELINE 'l'

#ifndef VECTOR_DISPLAY_SEND_DELAY
#define VECTOR_DISPLAY_SEND_DELAY 0
#endif

#define TFT_BLACK 0x0000       /*   0,   0,   0 */
#define TFT_NAVY 0x000F        /*   0,   0, 128 */
#define TFT_DARKGREEN 0x03E0   /*   0, 128,   0 */
#define TFT_DARKCYAN 0x03EF    /*   0, 128, 128 */
#define TFT_MAROON 0x7800      /* 128,   0,   0 */
#define TFT_PURPLE 0x780F      /* 128,   0, 128 */
#define TFT_OLIVE 0x7BE0       /* 128, 128,   0 */
#define TFT_LIGHTGREY 0xC618   /* 192, 192, 192 */
#define TFT_DARKGREY 0x7BEF    /* 128, 128, 128 */
#define TFT_BLUE 0x001F        /*   0,   0, 255 */
#define TFT_GREEN 0x07E0       /*   0, 255,   0 */
#define TFT_CYAN 0x07FF        /*   0, 255, 255 */
#define TFT_RED 0xF800         /* 255,   0,   0 */
#define TFT_MAGENTA 0xF81F     /* 255,   0, 255 */
#define TFT_YELLOW 0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE 0xFFFF       /* 255, 255, 255 */
#define TFT_ORANGE 0xFD20      /* 255, 165,   0 */
#define TFT_GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define TFT_PINK 0xF81F

// Color definitions for backwards compatibility
#define ILI9341_BLACK 0x0000       /*   0,   0,   0 */
#define ILI9341_NAVY 0x000F        /*   0,   0, 128 */
#define ILI9341_DARKGREEN 0x03E0   /*   0, 128,   0 */
#define ILI9341_DARKCYAN 0x03EF    /*   0, 128, 128 */
#define ILI9341_MAROON 0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE 0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE 0x7BE0       /* 128, 128,   0 */
#define ILI9341_LIGHTGREY 0xC618   /* 192, 192, 192 */
#define ILI9341_DARKGREY 0x7BEF    /* 128, 128, 128 */
#define ILI9341_BLUE 0x001F        /*   0,   0, 255 */
#define ILI9341_GREEN 0x07E0       /*   0, 255,   0 */
#define ILI9341_CYAN 0x07FF        /*   0, 255, 255 */
#define ILI9341_RED 0xF800         /* 255,   0,   0 */
#define ILI9341_MAGENTA 0xF81F     /* 255,   0, 255 */
#define ILI9341_YELLOW 0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE 0xFFFF       /* 255, 255, 255 */
#define ILI9341_ORANGE 0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define ILI9341_PINK 0xF81F

#define MESSAGE_DOWN 'D'
#define MESSAGE_UP 'U'
#define MESSAGE_MOVE 'M'
#define MESSAGE_BUTTON 'B'
#define MESSAGE_ACK 'A'

// These enumerate the text plotting alignment (reference datum point)
#define TL_DATUM 0    // Top left (default)
#define TC_DATUM 1    // Top centre
#define TR_DATUM 2    // Top right
#define ML_DATUM 3    // Middle left
#define CL_DATUM 3    // Centre left, same as above
#define MC_DATUM 4    // Middle centre
#define CC_DATUM 4    // Centre centre, same as above
#define MR_DATUM 5    // Middle right
#define CR_DATUM 5    // Centre right, same as above
#define BL_DATUM 6    // Bottom left
#define BC_DATUM 7    // Bottom centre
#define BR_DATUM 8    // Bottom right
#define L_BASELINE 9  // Left character baseline (Line the 'A' character would sit on)
#define C_BASELINE 10 // Centre character baseline
#define R_BASELINE 11 // Right character baseline

typedef uint32_t FixedPoint32;
#define TO_FP32(f) ((uint32_t)((f) * 65536. + 0.5))

struct VectorDisplayMessage {
    char what;
    char what2;
    union {
        uint8_t button;
        struct {
            int16_t x;
            int16_t y;
        } xy;
    } data;
} __attribute__((packed));

class VectorDisplayClass : public Print {
private:
    static const uint32_t MAX_BUFFER = (uint32_t)1024 * 256;
    static const uint32_t MESSAGE_TIMEOUT = 3000;
    static const uint8_t FLAG_LOW_ENDIAN_BITS = 1;
    static const uint8_t FLAG_HAVE_MASK = 2;
    static const uint8_t FLAG_PAD_BYTE = 4;
    static const uint8_t FLAG_LOW_ENDIAN_BYTES = 8;

    bool waitForAck = true;
    int gfxFontSize = 1;

    int readPos = 0;
    int32_t curForeColor565 = -1;
    uint32_t lastMessageStart = 0;
    int pointerX;
    int pointerY;
    int curWidth = VECTOR_DISPLAY_DEFAULT_WIDTH;
    int curHeight = VECTOR_DISPLAY_DEFAULT_HEIGHT;
    bool pointerDown = false;
    bool wrap = 1;
    bool fixCP437 = true;
    uint16_t polyLineCount;
    uint8_t polyLineSum;
    uint32_t delayTime = 0;

    uint8_t readBuf[VECTOR_DISPLAY_MESSAGE_SIZE];
    union {
        uint32_t color;
        uint16_t twoByte[9];
        struct {
            uint16_t x;
            uint16_t y;
            char text[VECTOR_DISPLAY_MAX_STRING + 1];
        } __attribute__((packed)) xyText;
        struct {
            uint16_t endianness;
            uint16_t width;
            uint16_t height;
            FixedPoint32 aspectRatio;
            uint16_t reserved[3];
        } __attribute__((packed)) initialize;
        struct {
            uint8_t c;
            char text[VECTOR_DISPLAY_MAX_STRING + 1];
        } __attribute__((packed)) charText;
        struct {
            uint16_t width;
            uint16_t height;
        } __attribute__((packed)) coords;
        struct {
            char attr;
            uint8_t value;
        } __attribute__((packed)) attribute8;
        struct {
            char attr;
            uint16_t value;
        } __attribute__((packed)) attribute16;
        struct {
            char attr;
            uint32_t value;
        } __attribute__((packed)) attribute32;
        struct {
            uint32_t length;
            uint8_t depth;
            uint8_t flags;
            uint16_t x;
            uint16_t y;
            uint16_t w;
            uint16_t h;
            uint32_t foreColor; // only if depth==1
            uint32_t backColor; // only if depth==1
        } __attribute__((packed)) bitmap;
        struct {
            uint16_t x1;
            uint16_t y1;
            uint16_t x2;
            uint16_t y2;
            uint16_t r;
            uint8_t filled;
        } __attribute__((packed)) roundedRectangle;
        struct {
            uint16_t x;
            uint16_t y;
            uint16_t r;
            FixedPoint32 angle1;
            FixedPoint32 sweep;
            uint8_t filled;
        } __attribute__((packed)) arc;
        struct {
            char attr;
            uint16_t values[2];
        } __attribute__((packed)) attribute16x2;
        uint8_t bytes[VECTOR_DISPLAY_MAX_STRING + 1];
        char text[VECTOR_DISPLAY_MAX_STRING + 1];
    } args;
    uint32_t lastSend = 0;

private:
    inline void sendDelay() {
        if (delayTime > 0) {
            while (millis() - lastSend < delayTime);
            lastSend = millis();
        }
    }

public:
    int cursor_x = 0;
    int cursor_y = 0;
    uint8_t rotation = 0;
    int textsize = 1;
    uint32_t textcolor = TFT_WHITE;
    uint32_t textbgcolor = TFT_BLACK;

    void setWaitForAck(bool wait) { waitForAck = wait; }

    void setDelay(uint32_t delayMillis) {
        delayTime = delayMillis;
        lastSend = millis();
    }

    virtual void remoteFlush() {
        /*while(remoteAvailable())
            remoteRead();
            * */
    }
    virtual int remoteRead() = 0; // must be non-blocking
    virtual void remoteWrite(uint8_t c) = 0;
    virtual void remoteWrite(const void *data, size_t n) = 0;
    virtual size_t remoteAvailable() = 0;

    inline SPIClass &getSPIinstance() { return SPI; }

    void attribute8(char a, uint8_t value) {
        args.attribute8.attr = a;
        args.attribute8.value = value;
        sendCommand('Y', &args, 2);
    }

    void attribute8(char a, bool value) {
        args.attribute8.attr = a;
        args.attribute8.value = value ? 1 : 0;
        sendCommand('Y', &args, 2);
    }

    void attribute16(char a, uint16_t value) {
        args.attribute16.attr = a;
        args.attribute16.value = value;
        sendCommand('A', &args, 3);
    }

    void attribute32(char a, uint32_t value) {
        args.attribute32.attr = a;
        args.attribute32.value = value;
        sendCommand('B', &args, 5);
    }

    void sendCommand(char c, const void *arguments, int argumentsLength) {
        sendDelay();
        remoteWrite(c);
        remoteWrite(c ^ 0xFF);
        if (argumentsLength > 0) remoteWrite((uint8_t *)arguments, argumentsLength);
        uint8_t sum = 0;
        for (int i = 0; i < argumentsLength; i++) sum += ((uint8_t *)arguments)[i];
        remoteWrite((uint8_t)(sum ^ 0xFF));
    }

    void sendCommandWithAck(char c, const void *arguments, int argumentsLength) {
        readPos = 0;
        bool done = false;
        do {
            uint32_t t0 = millis();

            sendCommand(c, arguments, argumentsLength);

            return;
            // if (!waitForAck)
            //     return;

            while ((millis() - t0) < 500) {
                if (readMessage(NULL) && !memcmp(readBuf, "Acknwld", 7) && readBuf[7] == c) {
                    done = true;
                    break;
                }
            }

        } while (!done);
    }

    uint16_t width() { return (rotation % 2) ? curHeight : curWidth; }

    uint16_t height() { return (rotation % 2) ? curWidth : curHeight; }

    int16_t fontHeight(void) { return 8 * gfxFontSize; }

    int16_t textWidth(const char *string) { return 5 * gfxFontSize; }
    int16_t textWidth(const char *string, uint8_t font) { return 5 * gfxFontSize; }

    uint8_t sumBytes(void *data, int length) {
        uint8_t *p = (uint8_t *)data;
        uint8_t s = 0;
        while (length-- > 0) s += *p++;
        return s;
    }

    void startPoly(char c, uint16_t n) {
        polyLineCount = n;
        remoteWrite(c);
        remoteWrite(c ^ 0xFF);
        args.twoByte[0] = n;
        remoteWrite((uint8_t *)&args, 2);
        polyLineSum = args.bytes[0] + args.bytes[1];
    }

    void startFillPoly(uint16_t n) { startPoly('N', n); }

    void startPolyLine(uint16_t n) { startPoly('O', n); }

    void addPolyLine(int16_t x, int16_t y) {
        if (polyLineCount > 0) {
            args.twoByte[0] = x;
            args.twoByte[1] = y;
            remoteWrite((uint8_t *)&args, 4);
            polyLineSum += args.bytes[0] + args.bytes[1] + args.bytes[2] + args.bytes[3];
            polyLineCount--;
            if (polyLineCount == 0) { remoteWrite(0xFF ^ polyLineSum); }
        }
    }

    void line(int x1, int y1, int x2, int y2) {
        args.twoByte[0] = x1;
        args.twoByte[1] = y1;
        args.twoByte[2] = x2;
        args.twoByte[3] = y2;
        sendCommand('L', &args, 8);
    }

    void fillRectangle(int x1, int y1, int x2, int y2) {
        args.twoByte[0] = x1;
        args.twoByte[1] = y1;
        args.twoByte[2] = x2;
        args.twoByte[3] = y2;
        sendCommand('R', &args, 8);
    }

    void invertDisplay(bool i) { delay(0); }

    void rectangle(int x1, int y1, int x2, int y2, bool fill = false) {
        if (fill) fillRectangle(x1, y1, x2, y2);
        else {
            startPolyLine(4);
            addPolyLine(x1, y1);
            addPolyLine(x2, y1);
            addPolyLine(x2, y2);
            addPolyLine(x1, y2);
        }
    }

    void roundedRectangle(int x1, int y1, int x2, int y2, int r, bool fill) {
        args.roundedRectangle.filled = fill ? 1 : 0;
        args.roundedRectangle.x1 = x1;
        args.roundedRectangle.x2 = x2;
        args.roundedRectangle.y1 = y1;
        args.roundedRectangle.y2 = y2;
        args.roundedRectangle.r = r;
        sendCommand('Q', &args, 11);
    }

    void roundedRectangle(int x1, int y1, int x2, int y2, int r) {
        roundedRectangle(x1, y1, x2, y2, r, false);
    }

    void fillRoundedRectangle(int x1, int y1, int x2, int y2, int r) {
        roundedRectangle(x1, y1, x2, y2, r, true);
    }

    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
        args.twoByte[0] = x1;
        args.twoByte[1] = y1;
        args.twoByte[2] = x2;
        args.twoByte[3] = y2;
        args.twoByte[4] = x3;
        args.twoByte[5] = y3;
        sendCommand('G', &args, 12);
    }

    /*    void initialize() {
            args.twoByte[0] = 0x1234; // endianness detector
            args.twoByte[1] = 0;
            sendCommandWithAck('H', &args, 4);
        } */

    void initialize(int w = VECTOR_DISPLAY_DEFAULT_WIDTH, int h = VECTOR_DISPLAY_DEFAULT_HEIGHT) {
        args.initialize.endianness = 0x1234; // endianness detector
        args.initialize.width = w;
        args.initialize.height = h;
        args.initialize.aspectRatio = TO_FP32(1.);
        args.initialize.reserved[0] = 0;
        args.initialize.reserved[1] = 0;
        args.initialize.reserved[2] = 0;
        curWidth = w;
        curHeight = h;

        sendCommandWithAck('Z', &args, 16);
    }

    void fillCircle(int x, int y, int r) {
        args.twoByte[0] = x;
        args.twoByte[1] = y;
        args.twoByte[2] = r;
        sendCommand('J', &args, 6);
    }

    void circle(int x, int y, int r) {
        args.twoByte[0] = x;
        args.twoByte[1] = y;
        args.twoByte[2] = r;
        sendCommand('I', &args, 6);
    }

    void point(int x, int y) {
        args.twoByte[0] = x;
        args.twoByte[1] = y;
        sendCommand('P', &args, 4);
    }

    void arc(int x, int y, int r, FixedPoint32 angle1, FixedPoint32 sweep, bool fill = false) {
        args.arc.x = x;
        args.arc.y = y;
        args.arc.r = r;
        args.arc.angle1 = angle1;
        args.arc.sweep = sweep;
        args.arc.filled = fill ? 1 : 0;
        sendCommand('S', &args, 15);
    }

    void arc(int x, int y, int r, float angle1, float sweep, bool fill = false) {
        arc(x, y, r, TO_FP32(angle1), TO_FP32(sweep), fill);
    }

    // 32-bit fixed point
    void textSize(FixedPoint32 s) {
        args.attribute32.attr = 's';
        args.attribute32.value = s;
        sendCommand('B', &args, 5);
    }

    void text(int x, int y, const char *str, int n) {
        args.xyText.x = x;
        args.xyText.y = y;
        if (n > VECTOR_DISPLAY_MAX_STRING) n = VECTOR_DISPLAY_MAX_STRING;
        strncpy(args.xyText.text, str, n);

        if (fixCP437) {
            for (int i = 0; i < n; i++) {
                if ((uint8_t)args.xyText.text[i] >= 176) args.xyText.text[i]++;
            }
        }
        args.xyText.text[n] = 0;
        sendCommand('T', &args, 4 + strlen(args.xyText.text) + 1);
    }

    void text(int x, int y, const char *str) { text(x, y, str, strlen(str)); }

    void text(int x, int y, String str) { text(x, y, str.c_str(), str.length()); }

    void deleteButton(uint8_t command) { sendCommand('D', &command, 1); }

    void addButton(uint8_t command, const char *str) {
        args.charText.c = command;
        strncpy(args.charText.text, str, VECTOR_DISPLAY_MAX_STRING);
        args.charText.text[VECTOR_DISPLAY_MAX_STRING] = 0;
        sendCommand('U', &args, 1 + strlen(args.charText.text) + 1);
    }

    void addButton(uint8_t command, String str) { addButton(command, str.c_str()); }

    void toast(const char *str, unsigned n) {
        if (VECTOR_DISPLAY_MAX_STRING < n) n = VECTOR_DISPLAY_MAX_STRING;
        strncpy(args.text, str, n);
        args.text[n] = 0;
        sendCommand('M', &args, n + 1);
    }

    void toast(const char *str) { toast(str, strlen(str)); }

    void toast(String text) { toast(text.c_str(), text.length()); }

    void foreColor(uint32_t color) {
        args.attribute32.attr = 'f';
        args.attribute32.value = color;
        sendCommand('B', &args, 5);
        curForeColor565 = -1;
    }

    void backColor(uint32_t color) {
        args.attribute32.attr = 'b';
        args.attribute32.value = color;
        sendCommand('B', &args, 5);
    }

    void textBackColor(uint32_t color) {
        args.attribute32.attr = 'k';
        args.attribute32.value = color;
        sendCommand('B', &args, 5);
    }

    void textForeColor(uint32_t color) {
        args.attribute32.attr = 'F';
        args.attribute32.value = color;
        sendCommand('B', &args, 5);
    }

    void foreColor565(uint16_t color) {
        args.attribute16.attr = 'f';
        args.attribute16.value = color;
        sendCommand('A', &args, 3);
        curForeColor565 = color;
    }

    void backColor565(uint16_t color) {
        args.attribute16.attr = 'b';
        args.attribute16.value = color;
        sendCommand('A', &args, 3);
    }

    void textBackColor565(uint16_t color) {
        args.attribute16.attr = 'k';
        args.attribute16.value = color;
        sendCommand('A', &args, 3);
    }

    void textForeColor565(uint16_t color) {
        args.attribute16.attr = 'F';
        args.attribute16.value = color;
        sendCommand('A', &args, 3);
    }

    void rounded(uint8_t value) {
        args.attribute8.attr = 'n';
        args.attribute8.value = value ? 1 : 0;
        sendCommand('Y', &args, 2);
    }

    void thickness(FixedPoint32 t) {
        args.attribute32.attr = 't';
        args.attribute32.value = t;
        sendCommand('B', &args, 5);
    }

    void pixelAspectRatio(FixedPoint32 a) {
        args.attribute32.attr = 'a';
        args.attribute32.value = a;
        sendCommand('B', &args, 5);
    }

#ifdef SUPPORT_FLOATING_POINT
    inline void setThickness(double thickness) { setThickness(TO_FP32(thickness)); }

    inline void setPixelAspectRatio(double aspect) { setThickness(TO_FP32(aspect)); }
#endif

    void clear() { sendCommand('C', NULL, 0); }

    void update() { sendCommand('F', NULL, 0); }

    /*    void reset() {
            sendCommandWithAck('E', NULL, 0);
        } */

    void coordinates(int width, int height) {
        args.attribute16x2.attr = 'c';
        curWidth = width;
        curHeight = height;
        args.attribute16x2.values[0] = width;
        args.attribute16x2.values[1] = height;
        sendCommandWithAck('B', &args, 5);
    }

    void continuousUpdate(bool value) {
        args.attribute8.attr = 'c';
        args.attribute8.value = value ? 1 : 0;
        sendCommand('Y', &args, 2);
    }

    void textHorizontalAlign(char hAlign) {
        args.attribute8.attr = 'h';
        args.attribute8.value = hAlign;
        sendCommand('Y', &args, 2);
    }

    void textVerticalAlign(char hAlign) {
        args.attribute8.attr = 'v';
        args.attribute8.value = hAlign;
        sendCommand('Y', &args, 2);
    }

    void textOpaqueBackground(bool opaque) {
        args.attribute8.attr = 'o';
        args.attribute8.value = opaque ? 1 : 0;
        sendCommand('Y', &args, 2);
    }

    void textBold(bool bold) {
        args.attribute8.attr = 'b';
        args.attribute8.value = bold ? 1 : 0;
        sendCommand('Y', &args, 2);
    }

    bool isTouchDown() { return pointerDown; }

    int getTouchX() { return pointerX; }

    int getTouchY() { return pointerY; }

    bool readMessage(VectorDisplayMessage *msg) {
        while (remoteAvailable()) {
            uint8_t c = remoteRead();

            if (0 < readPos && millis() - lastMessageStart > MESSAGE_TIMEOUT) readPos = 0;

            if (2 <= readPos) {
                readBuf[readPos++] = c;
                if (readPos >= VECTOR_DISPLAY_MESSAGE_SIZE) {
                    readPos = 0;
                    if (msg != NULL) memcpy(msg, readBuf, sizeof(VectorDisplayMessage));
                    else msg = (VectorDisplayMessage *)readBuf;

                    if (msg->what == MESSAGE_DOWN || msg->what == MESSAGE_UP || msg->what == MESSAGE_MOVE) {
                        pointerDown = msg->what != MESSAGE_UP;
                        pointerX = msg->data.xy.x;
                        pointerY = msg->data.xy.y;
                    }
                    return true;
                }
                continue;
            }

            if (1 <= readPos) {
                if ((*readBuf == 'U' && c == 'P') || (*readBuf == 'D' && c == 'N') ||
                    (*readBuf == 'M' && c == 'V') || (*readBuf == 'B' && c == 'T') ||
                    (*readBuf == 'A' && c == 'c')) {
                    readBuf[readPos++] = c;
                    continue;
                }
                readPos = 0;
            }
            if (readPos == 0 && (c == 'U' || c == 'D' || c == 'M' || c == 'B' || c == 'A')) {
                readBuf[readPos++] = c;
                lastMessageStart = millis();
            }
        }
        return false;
    }

    uint32_t color565To8888(uint16_t c) {
        return 0xFF000000 | ((((c >> 11) & 0x1F) * 255 / 0x1F) << 16) |
               ((((c >> 5) & 0x3F) * 255 / 0x3F) << 8) | ((c & 0x1F) * 255 / 0x1F);
    }

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    uint32_t getBitmap1Size(int16_t w, int16_t h, uint8_t flags = 0) {
        return (flags & FLAG_PAD_BYTE) ? ((uint32_t)w + 7) / 8 * h : ((uint32_t)w * h + 7) / 8;
    }

    uint32_t getBitmapSize(int16_t w, int16_t h, uint8_t depth = 1, uint8_t flags = 0) {
        if (depth == 1) {
            return getBitmap1Size(w, h, flags);
        } else {
            return w * h * (depth / 8);
        }
    }

    /*TODO: stubs*/
    void *createSprite(int16_t width, int16_t height, uint8_t frames = 1) { return NULL; }
    void pushSprite(int32_t x, int32_t y) {}
    void deleteSprite(void) {}
    void fillSprite(uint32_t color) {}

    void bitmap_progmem(
        int16_t x, int16_t y, const uint8_t *bmp, int16_t w, int16_t h, uint8_t depth = 1, uint8_t flags = 0,
        const uint8_t *mask = NULL, uint32_t foreColor = 0xFFFFFFFF, uint32_t backColor = 0x00FFFFFF
    ) /* PROGMEM */ {
        if (mask != NULL) flags |= FLAG_HAVE_MASK;
        uint32_t bitmapSize = getBitmapSize(w, h, depth, flags);
        int headerSize = depth == 1 ? 22 : 14;
        uint32_t maskSize = mask == NULL ? 0 : getBitmap1Size(w, h, flags);
        uint32_t fullSize = bitmapSize + headerSize + maskSize;

        if (fullSize + 1 > MAX_BUFFER) return;

        sendDelay();
        remoteWrite('K');
        remoteWrite('K' ^ 0xFF);
        args.bitmap.length = fullSize;
        args.bitmap.depth = 1;
        args.bitmap.flags = flags;
        args.bitmap.x = x;
        args.bitmap.y = y;
        args.bitmap.w = w;
        args.bitmap.h = h;
        if (depth == 1) {
            args.bitmap.foreColor = foreColor;
            args.bitmap.backColor = backColor;
        }

        uint8_t sum = sumBytes(&args, headerSize);
        remoteWrite(&args, headerSize);
        for (uint32_t i = 0; i < bitmapSize; i++) {
            uint8_t c = pgm_read_byte_near(bmp + i);
            remoteWrite(c);
            sum += c;
        }
        for (uint32_t i = 0; i < maskSize; i++) {
            uint8_t c = pgm_read_byte_near(mask + i);
            remoteWrite(c);
            sum += c;
        }
        remoteWrite(sum ^ 0xFF);
    }

    void bitmap(
        int16_t x, int16_t y, uint8_t *bmp, int16_t w, int16_t h, uint8_t depth, uint8_t flags = 0,
        uint8_t *mask = NULL, uint32_t foreColor = 0xFFFFFFFF, uint32_t backColor = 0x00FFFFFF
    ) {
        if (mask != NULL) flags |= FLAG_HAVE_MASK;
        uint32_t bitmapSize = getBitmapSize(w, h, depth, flags);
        int headerSize = depth == 1 ? 22 : 14;
        uint32_t maskSize = mask == NULL ? 0 : getBitmap1Size(w, h, flags);
        uint32_t fullSize = bitmapSize + (headerSize - 14) + maskSize;

        if (fullSize + 1 > MAX_BUFFER) return;

        sendDelay();
        remoteWrite('K');
        remoteWrite('K' ^ 0xFF);
        args.bitmap.length = fullSize;
        args.bitmap.depth = depth;
        args.bitmap.flags = flags;
        args.bitmap.x = x;
        args.bitmap.y = y;
        args.bitmap.w = w;
        args.bitmap.h = h;
        if (depth == 1) {
            args.bitmap.foreColor = foreColor;
            args.bitmap.backColor = backColor;
        }
        remoteWrite(&args, headerSize);
        remoteWrite(bmp, bitmapSize);
        uint8_t sum = sumBytes(&args, headerSize) + sumBytes((void *)bmp, bitmapSize);
        if (maskSize > 0) {
            remoteWrite(mask, maskSize);
            sum += sumBytes((void *)mask, maskSize);
        }
        remoteWrite(sum ^ 0xFF);
    }

    void utf8() {
        fixCP437 = false;
        args.attribute8.attr = 'i';
        args.attribute8.value = 0;
        sendCommand('Y', &args, 2);
    }

    /* The following are meant to be compatible with Adafruit GFX */
    void cp437(bool s) {
        // if true, activates real cp437 mode; if false, activates buggy Arduino compatible cp437 mode
        fixCP437 = !s;
        args.attribute8.attr = 'i';
        args.attribute8.value = 1;
        sendCommand('Y', &args, 2);
    }

    void setRotation(uint8_t r) {
        args.attribute8.attr = 'r';
        args.attribute8.value = r;
        rotation = r & 3;
        sendCommand('Y', &args, 2);
    }
    void setTextSize(uint8_t size) {
        gfxFontSize = size;
        textsize = size;
        textSize((FixedPoint32)size * 8 * 65536);
    }
    void setTextFont(uint8_t font) { setTextSize(font); }

    void setTextDatum(uint8_t d) {} // mockup

    void setTextColor(uint16_t f, uint16_t b) {
        textBackColor565(b);
        textForeColor565(f);
        textcolor = f;
        textbgcolor = b;
        textOpaqueBackground(true);
    }

    void setTextColor(uint16_t f) {
        textForeColor565(f);
        textcolor = f;
        textOpaqueBackground(false);
    }

    void setCursor(int16_t x, int16_t y) {
        cursor_x = x;
        cursor_y = y;
    }

    int16_t getCursorX(void) { return cursor_x; }

    int16_t getCursorY(void) { return cursor_y; }

    void setTextWrap(bool x, bool wrapY = false) { wrap = x; }

    int16_t drawRightString(const char *string, int32_t x, int32_t y, uint8_t font) {
        // TODO: add spaces
        return drawString(string, x, y);
    }

    int16_t drawRightString(const String &string, int32_t x, int32_t y, uint8_t font) {
        return drawRightString(string.c_str(), x, y, font);
    }

    int16_t drawCentreString(const char *string, int32_t x, int32_t y, uint8_t font) {
        // TODO: add spaces
        return drawString(string, x, y);
    }

    int16_t drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font) {
        return drawCentreString(string.c_str(), x, y, font);
    }

    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font) {
        return drawString(string.c_str(), x, y);
    }

    int16_t drawString(const char *string, int32_t x, int32_t y) {
        setCursor(x, y);
        return 0;
        write(string);
    };

    int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y) {
        setCursor(x, y);
        return 0;
        write(uniCode);
    }

    // TODO: fix back color handling
    size_t write(uint8_t c) override {
        if (c == '\n') {
            cursor_y += 8 * gfxFontSize;
            cursor_x = 0;
        }
        if (wrap && cursor_x + 5 * gfxFontSize > width()) {
            cursor_x = 0;
            cursor_y += 8 * gfxFontSize;
        }
        text(cursor_x, cursor_y, (char *)&c, 1);
        cursor_x += 5 * gfxFontSize;
        return 0;
    }

    // TODO: fix back color handling
    size_t write(const char *s) {
        int l = strlen(s);
        char *clean = (char *)malloc(l + 1);
        if (!clean) return 0;
        // Remove '\n' from string
        int j = 0;
        for (int i = 0; i < l; ++i) {
            if (s[i] != '\n') clean[j++] = s[i];
        }
        clean[j] = '\0';

        int w = width();
        int len = j;
        if (!wrap || cursor_x + 5 * gfxFontSize * len <= w) {
            text(cursor_x, cursor_y, clean);
            cursor_x += 5 * gfxFontSize * len;
        } else {
            const char *p = clean;
            while (len > 0) {
                int end = ((int)w - cursor_x) / (5 * gfxFontSize);
                if (end <= 0) {
                    cursor_x = 0;
                    cursor_y += 8 * gfxFontSize;
                    end = w / (5 * gfxFontSize);
                }
                if (end > len) end = len;
                text(cursor_x, cursor_y, p, end);
                p += end;
                len -= end;
                cursor_x = 5 * gfxFontSize * end;
            }
        }
        free(clean);
        return 0;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        point(x, y);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        rectangle(x, y, x + w - 1, y + h - 1);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        fillRectangle(x, y, x + w - 1, y + h - 1);
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        line(x, y, x + w, y);
    }

    void drawLine(int16_t x, int16_t y, int16_t x2, int16_t y2, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        line(x, y, x2, y2);
    }

    // Draw an anti-aliased wide line from ax,ay to bx,by width wd with radiused ends (radius is wd/2)
    // If bg_color is not included the background pixel colour will be read from TFT or sprite
    void drawWideLine(
        float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF
    ) {
        drawRect(ax, ay, wd, abs(ay - by), fg_color);
    }

    // As per "drawSmoothArc" except the ends of the arc are NOT anti-aliased, this facilitates dynamic arc
    // length changes with arc segments and ensures clean segment joints. The sides of the arc are
    // anti-aliased by default. If smoothArc is false sides will NOT be anti-aliased
    void drawArc(
        int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle,
        uint32_t fg_color, uint32_t bg_color, bool smoothArc = true
    ) {
        // TODO
        drawRect(x, y, r * 2, ir * 2, fg_color);
    }

    void drawSmoothArc(
        int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle,
        uint32_t fg_color, uint32_t bg_color, bool roundEnds = false
    ) {
        drawArc(x, y, r, ir, startAngle, endAngle, fg_color, bg_color);
    }

    // Draw an anti-aliased filled circle at x, y with radius r
    // If bg_color is not included the background pixel colour will be read from TFT or sprite
    void fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color = 0x00FFFFFF) {
        fillCircle(x, y, r, color);
    }

    void drawSmoothRoundRect(
        int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color,
        uint32_t bg_color = 0x00FFFFFF, uint8_t quadrants = 0xF
    ) {
        drawRoundRect(x, y, w, h, r, fg_color);
    }

    // Draw a filled rounded rectangle , corner radius r and bounding box defined by x,y and w,h
    void fillSmoothRoundRect(
        int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color,
        uint32_t bg_color = 0x00FFFFFF
    ) {
        fillRoundRect(x, y, w, h, radius, color);
    }

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        line(x, y, x, y + h);
    }

    void fillScreen(uint16_t color) {
        backColor565(color);
        clear();
        backColor(0xFF000000);
    }

    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        circle(x, y, r);
    }

    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        fillCircle(x, y, r);
    }

    void drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {}
    void fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color) {
        // TODO
        if (rx < ry) fillCircle(x, y, rx, color);
        else fillCircle(x, y, ry, color);
    }

    virtual void begin(int width = VECTOR_DISPLAY_DEFAULT_WIDTH, int height = VECTOR_DISPLAY_DEFAULT_HEIGHT) {
        remoteFlush();
        initialize(width, height);
    }

    virtual void end() {}

    void
    fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        fillTriangle(x0, y0, x1, y1, x2, y2);
    }

    void
    drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        line(x0, y0, x1, y1);
        line(x1, y1, x2, y2);
        line(x2, y2, x0, y0);
    }

    void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        roundedRectangle(x0, y0, w, h, radius, false);
    }

    void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        roundedRectangle(x0, y0, w, h, radius, true);
    }

    void drawBitmap(
        int16_t x, int16_t y, const uint8_t bmp[], int16_t w, int16_t h, uint16_t color
    ) /* PROGMEM */ {
        bitmap_progmem(x, y, bmp, w, h, 1, 0, NULL, color565To8888(color), 0); // transparent background
    }

    void drawBitmap(int16_t x, int16_t y, uint8_t *bmp, int16_t w, int16_t h, uint16_t color) {
        bitmap(x, y, bmp, w, h, 1, 0, NULL, color565To8888(color), 0); // transparent background
    }

    void
    drawBitmap(int16_t x, int16_t y, const uint8_t bmp[], int16_t w, int16_t h, uint16_t color, uint16_t bg) {
        bitmap_progmem(x, y, bmp, w, h, 1, 0, NULL, color565To8888(color), color565To8888(bg));
    }

    void drawBitmap(int16_t x, int16_t y, uint8_t *bmp, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
        bitmap(x, y, bmp, w, h, 1, 0, NULL, color565To8888(color), color565To8888(bg));
    }

    void drawXBitmap(int16_t x, int16_t y, const uint8_t bmp[], int16_t w, int16_t h, uint16_t color) {
        bitmap_progmem(
            x, y, bmp, w, h, 1, FLAG_PAD_BYTE | FLAG_LOW_ENDIAN_BITS, NULL, color565To8888(color), 0
        );
    }

    void drawXBitmap(
        int16_t x, int16_t y, const uint8_t bmp[], int16_t w, int16_t h, uint16_t color, uint16_t bgcolor
    ) {
        bitmap_progmem(
            x,
            y,
            bmp,
            w,
            h,
            1,
            FLAG_PAD_BYTE | FLAG_LOW_ENDIAN_BITS,
            NULL,
            color565To8888(color),
            color565To8888(bgcolor)
        );
    }

    void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bmp[], int16_t w, int16_t h) {
        bitmap_progmem(x, y, bmp, w, h, 8, 0, NULL);
    }

    void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bmp, int16_t w, int16_t h) {
        bitmap(x, y, bmp, w, h, 8, 0, NULL);
    }

    void drawGrayscaleBitmap(
        int16_t x, int16_t y, const uint8_t bmp[], const uint8_t mask[], int16_t w, int16_t h
    ) {
        bitmap_progmem(x, y, bmp, w, h, 8, 0, mask);
    }

    void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bmp, uint8_t *mask, int16_t w, int16_t h) {
        bitmap(x, y, bmp, w, h, 8, 0, mask);
    }

    void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bmp, int16_t w, int16_t h) {
        bitmap(x, y, (uint8_t *)bmp, w, h, 16, FLAG_LOW_ENDIAN_BYTES, NULL);
    }

    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bmp[], int16_t w, int16_t h) {
        bitmap_progmem(x, y, (const uint8_t *)bmp, w, h, 16, FLAG_LOW_ENDIAN_BYTES, NULL);
    }

    void
    drawRGBBitmap(int16_t x, int16_t y, const uint16_t bmp[], const uint8_t mask[], int16_t w, int16_t h) {
        bitmap_progmem(x, y, (const uint8_t *)bmp, w, h, 16, FLAG_LOW_ENDIAN_BYTES, (const uint8_t *)mask);
    }

    void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bmp, uint8_t *mask, int16_t w, int16_t h) {
        bitmap(x, y, (uint8_t *)bmp, w, h, 16, FLAG_LOW_ENDIAN_BYTES, mask);
    }

    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
        if (color != curForeColor565) { foreColor565(color); }
        if (cornername & 0x1) { arc(x0, y0, r, TO_FP32(180), TO_FP32(90), false); }
        if (cornername & 0x2) { arc(x0, y0, r, TO_FP32(270), TO_FP32(90), false); }
        if (cornername & 0x4) { arc(x0, y0, r, TO_FP32(0), TO_FP32(90), false); }
        if (cornername & 0x8) { arc(x0, y0, r, TO_FP32(90), TO_FP32(90), false); }
    }

    void fillCircleHelper(int16_t cx, int16_t cy, int16_t r, uint8_t corners, int16_t delta, uint16_t color) {
        /* This is not efficient, but it's there just for completeness and I don't
        know that much code will actually call this. The Adafruit GFX library uses
        this for rounded rectangles, but we are doing rounded rectangles directly
        via the Android function so we don't need this. */
        if ((corners & 3) == 0) return;

        if (color != curForeColor565) { foreColor565(color); }

        line(cx, cy - r, cx, cy + r + delta);
        if (corners & 2) {
            arc(cx, cy, r, TO_FP32(90), TO_FP32(180), false);
            arc(cx, cy, r, TO_FP32(90), TO_FP32(180), true);
            arc(cx, cy + delta, r, TO_FP32(90), TO_FP32(180), false);
            arc(cx, cy + delta, r, TO_FP32(90), TO_FP32(180), true);
            if (delta > 0) {
                rectangle(cx - r, cy, cx, cy + delta, false);
                rectangle(cx - r, cy, cx, cy + delta, true);
            }
        }
        if (corners & 1) {
            arc(
                cx, cy, r, TO_FP32(270), TO_FP32(180), false
            ); // drawing edges separately makes things fit better
            arc(cx, cy, r, TO_FP32(270), TO_FP32(180), true);
            arc(cx, cy + delta, r, TO_FP32(270), TO_FP32(180), false);
            arc(cx, cy + delta, r, TO_FP32(270), TO_FP32(180), true);
            if (delta > 0) {
                rectangle(cx, cy, cx + r, cy + delta, false);
                rectangle(cx, cy, cx + r, cy + delta, true);
            }
        }
    }

    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data) {};
    void pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transparent) {};

    void setAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h) {};
    void pushPixels(const void *data_in, uint32_t len) {};

    void startWrite(void) {};
    void endWrite(void) {};

    /* the following Adafruit GFX APIs are not implemented at present */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {}
    void setFont(const void /*GFXfont*/ *f = NULL) {}
    void getTextBounds(
        const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h
    ) {}
    void getTextBounds(
        const void /*__FlashStringHelper*/ *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w,
        uint16_t *h
    ) {}
#pragma GCC diagnostic pop
};

class SerialDisplayClass : public VectorDisplayClass {
private:
    Stream &s;
    const bool doSerialBegin;

public:
    virtual int remoteRead() override {
        // return s.read();
        return ('A');
    }

    virtual void remoteWrite(uint8_t c) override {
        // s.write(c);
    }

    virtual void remoteWrite(const void *data, size_t n) override {
        // s.write((uint8_t*)data, n);
    }

    /* only works with the Serial object; do not call externally without it */
    void begin(
        uint32_t speed, int width = VECTOR_DISPLAY_DEFAULT_WIDTH, int height = VECTOR_DISPLAY_DEFAULT_HEIGHT
    ) {
#ifndef NO_SERIAL
        if (doSerialBegin) {
            Serial.begin(speed);
            while (!Serial);
        }
#endif
        VectorDisplayClass::begin(width, height);
    }

    bool getSwapBytes(void) { return false; } // stub
    void setSwapBytes(bool swap) { return; }  // stub

    virtual void
    begin(int width = VECTOR_DISPLAY_DEFAULT_WIDTH, int height = VECTOR_DISPLAY_DEFAULT_HEIGHT) override {
        begin(115200, width, height);
    }

    virtual size_t remoteAvailable() override { return s.available(); }

#ifndef NO_SERIAL
    SerialDisplayClass(int16_t w = TFT_WIDTH, int16_t h = TFT_HEIGHT) : s(Serial), doSerialBegin(true) {}
#endif

    SerialDisplayClass(Stream &_s) : s(_s), doSerialBegin(false) {}
};

#ifdef ESP8266
class WiFiDisplayClass : public SerialDisplayClass {
private:
    WiFiClient client;

public:
    bool begin(
        const char *host, int width = VECTOR_DISPLAY_DEFAULT_WIDTH, int height = VECTOR_DISPLAY_DEFAULT_HEIGHT
    ) {
        VectorDisplayClass::begin(width, height);
        return client.connect(host, 7788);
    }

    virtual void end() override {
        VectorDisplayClass::end();
        client.stop();
    }

    WiFiDisplayClass() : SerialDisplayClass(client) {}
};
#endif

#endif
