#pragma once
#include <LovyanGFX.hpp>

#if defined(LGFX_USE_QSPI)

namespace lgfx {

// Hardware back-end: QSPI plumbing (0x02 cmd / 0x32 pixel opcodes) from
// Panel_ST77916 plus the ST77922 vendor init. Never drawn to directly: the
// panel needs window x/width aligned to 4 pixels, so Panel_ST77922 below keeps
// a framebuffer and flushes aligned rectangles through this class.
class Panel_ST77922HW : public Panel_ST77916 {
public:
    Panel_ST77922HW() : Panel_ST77916() {
        _cfg.panel_width  = _cfg.memory_width  = 320;
        _cfg.panel_height = _cfg.memory_height = 480;
    }

    void qspiBegin() { start_qspi(); }
    void qspiEnd() { end_qspi(); }

    // Vendor BSP ends up with COLMOD=0x01 for 16bpp (init table overrides 0x55).
    color_depth_t setColorDepth(color_depth_t) override {
        color_depth_t depth = rgb565_2Byte;
        _write_depth = _read_depth = depth;
        _write_bits = _read_bits = ((int)depth & color_depth_t::bit_mask);
        startWrite();
        cs_control(false);
        write_cmd(0x3A);
        _bus->writeCommand(0x01, 8);
        _bus->wait();
        cs_control(true);
        endWrite();
        return _write_depth;
    }

protected:
    const uint8_t *getInitCommands(uint8_t listno) const override {
        // Format: cmd, len[|0x80 for delay], params..., [delay_ms if 0x80]
        // Terminated by 0xFF, 0xFF.
        // MADCTL (0x36), COLMOD (0x3A) are sent by Panel_ST77916::init() after
        // this sequence runs; do NOT include them here.
        static constexpr uint8_t list0[] = {
            0xF1, 1,  0x00,
            0x60, 3,  0x00, 0x00, 0x00,
            0x65, 1,  0x80,
            0x79, 1,  0x06,
            0x7B, 3,  0x00, 0x08, 0x08,
            0x80, 11, 0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA,
            0x81, 4,  0x26, 0x52, 0x72, 0x27,
            0x84, 2,  0x92, 0x25,
            0x87, 6,  0x10, 0x10, 0x58, 0x00, 0x02, 0x3A,
            0x88, 15, 0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00,
                      0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06,
            0x89, 3,  0x00, 0x00, 0x00,
            0x8A, 11, 0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19,
            0x8B, 9,  0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E,
            0x8C, 13, 0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50,
                      0x0F, 0x01, 0xC5, 0x12, 0x09,
            0x8D, 1,  0x0C,
            0x8E, 6,  0x33, 0x01, 0x0C, 0x13, 0x01, 0x01,
            0xB3, 2,  0x00, 0x30,
            0xF1, 1,  0x00,
            0x71, 1,  0xD0,
            0x66, 2,  0x02, 0x3F,
            0xBE, 3,  0x26, 0x00, 0x9D,
            0x70, 12, 0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00, 0x00, 0x1A,
            0x90, 9,  0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27,
            0x91, 9,  0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27,
            0x92, 10, 0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27,
            0x93, 10, 0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27,
            0x94, 6,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x95, 5,  0x96, 0x16, 0x00, 0x00, 0xFF,
            0x96, 12, 0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27, 0x00, 0x44,
            0x97, 12, 0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27, 0x00, 0x44,
            0xBA, 5,  0x55, 0x94, 0x2D, 0x94, 0x27,
            0x9A, 7,  0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
            0x9B, 7,  0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
            0x9C, 13, 0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00,
                      0x10, 0x02, 0x00, 0x00, 0x00,
            0x9D, 8,  0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01,
            0x9E, 7,  0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01,
            0xB4, 12, 0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00, 0x1E, 0x10,
            0xB5, 12, 0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01, 0x1E, 0x12,
            0xB6, 7,  0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF,
            0x86, 14, 0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C,
                      0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5,
            0xB7, 16, 0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03,
                      0x03, 0x42, 0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D,
            0xB8, 16, 0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02,
                      0x03, 0x42, 0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D,
            0xB9, 2,  0x23, 0x23,
            0xBF, 6,  0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B,
            0xF2, 1,  0x00,
            0x73, 5,  0x04, 0xDA, 0x12, 0x54, 0x47,
            0x77, 5,  0x6B, 0x5B, 0xFD, 0xC3, 0xC5,
            0x7A, 2,  0x15, 0x27,
            0x7B, 2,  0x04, 0x57,
            0x7E, 2,  0x01, 0x0E,
            0xBF, 1,  0x36,
            0xE3, 2,  0x40, 0x40,
            0xF0, 1,  0x00,
            0xD0, 1,  0x00,
            // Sleep out; 120 ms delay required before display-on
            0x11, 1|0x80, 0x00, 120,
            // Display on
            0x29, 1,     0x00,
            0xFF, 0xFF,  // end of table
        };
        switch (listno) {
            case 0: return list0;
            default: return nullptr;
        }
    }
};

// Public panel: draws into a PSRAM framebuffer (rotation handled in software
// by Panel_FrameBufferBase) and flushes the dirty rectangle widened to a
// multiple of 4 pixels in x, which is what the ST77922 requires.
class Panel_ST77922 : public Panel_FrameBufferBase {
public:
    Panel_ST77922() {
        _cfg.panel_width  = _cfg.memory_width  = 320;
        _cfg.panel_height = _cfg.memory_height = 480;
    }

    bool init(bool use_reset) override {
        constexpr int W = 320, H = 480;
        _auto_display = true;
        if (!_fb) {
            _fb = (uint8_t *)heap_caps_malloc(W * H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            _lines_buffer = (uint8_t **)heap_caps_malloc(H * sizeof(uint8_t *), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (!_fb || !_lines_buffer) return false;
            for (int y = 0; y < H; y++) _lines_buffer[y] = _fb + y * W * 2;
        }
        memset(_fb, 0, W * H * 2);
        _write_depth = _read_depth = rgb565_2Byte;
        _write_bits = _read_bits = 16;

        _hw.config(_cfg);
        _hw.setBus(_bus);
        if (!_hw.init(use_reset)) return false;
        _hw.setRotation(0);
        _hw.setInvert(!_invert);

        setRotation(_rotation);
        _range_mod.left = 0; _range_mod.right = W - 1; _range_mod.top = 0; _range_mod.bottom = H - 1;
        display(0, 0, 0, 0);
        return true;
    }

    color_depth_t setColorDepth(color_depth_t) override { return rgb565_2Byte; }

    // INVON is this panel's normal state, so the logical "invert" is flipped.
    void setInvert(bool invert) override {
        _invert = invert;
        _hw.setInvert(!invert);
    }

    void display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h) override {
        if (!_fb) return;
        if (0 < w && 0 < h) {
            uint_fast8_t r = _internal_rotation;
            if (r) {
                if ((1u << r) & 0b10010110) { y = _height - (y + h); }
                if (r & 2)                  { x = _width  - (x + w); }
                if (r & 1) { std::swap(x, y); std::swap(w, h); }
            }
            _range_mod.left   = std::min<int_fast16_t>(_range_mod.left,   x);
            _range_mod.right  = std::max<int_fast16_t>(_range_mod.right,  x + w - 1);
            _range_mod.top    = std::min<int_fast16_t>(_range_mod.top,    y);
            _range_mod.bottom = std::max<int_fast16_t>(_range_mod.bottom, y + h - 1);
        }
        if (_range_mod.empty()) return;

        int xs = _range_mod.left & ~3;
        int xe = std::min<int>(_range_mod.right | 3, 319);
        int ys = std::max<int>(_range_mod.top, 0);
        int ye = std::min<int>(_range_mod.bottom, 479);
        _range_mod.top = INT16_MAX; _range_mod.left = INT16_MAX;
        _range_mod.right = 0;       _range_mod.bottom = 0;
        if (xe < xs || ye < ys) return;

        const uint32_t wb = (xe - xs + 1) * 2;
        auto bus = _hw.getBus();
        _hw.startWrite();
        _hw.setWindow(xs, ys, xe, ye);
        uint8_t *buf[2] = {bus->getDMABuffer(wb), bus->getDMABuffer(wb)};
        _hw.qspiBegin();
        if (buf[0] && buf[1]) {
            for (int y = ys, i = 0; y <= ye; y++, i++) {
                uint8_t *lb = buf[i & 1];
                memcpy(lb, &_lines_buffer[y][xs * 2], wb);
                bus->writeBytes(lb, wb, true, true);
            }
        } else {
            for (int y = ys; y <= ye; y++) bus->writeBytes(&_lines_buffer[y][xs * 2], wb, true, false);
        }
        bus->wait();
        _hw.qspiEnd();
        _hw.endWrite();
    }

private:
    Panel_ST77922HW _hw;
    uint8_t *_fb = nullptr;
};

}  // namespace lgfx

#endif  // LGFX_USE_QSPI
