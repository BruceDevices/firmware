"""Host regression for the Plus V1 AW9364 implementation (no device access)."""

from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
BOARD = ROOT / "boards/lilygo-t-embed-cc1101"


class PulseBacklightTest(unittest.TestCase):
    def test_driver_transitions(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.fail("Host C++ compiler (g++ or clang++) is required")
        source = (BOARD / "interface.cpp").read_text(encoding="utf-8")
        start = source.index("namespace {\n// Plus V1")
        end = source.index("} // namespace", start) + len("} // namespace")
        driver = source[start:end]
        harness = r'''
#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>
#define TFT_BL 21
#define OUTPUT 1
#define LOW 0
#define HIGH 1
#define portMAX_DELAY 0
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
struct StaticSemaphore_t { std::mutex mutex; };
using SemaphoreHandle_t = StaticSemaphore_t *;
bool locked = false, critical = false, started = false;
uint32_t nowUs = 0, lowAt = 0, highAt = 0;
unsigned int writes = 0;
int output = LOW, decodedStep = 0;
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *s) { return s; }
void xSemaphoreTake(SemaphoreHandle_t s, int) { s->mutex.lock(); assert(!locked); locked = true; }
void xSemaphoreGive(SemaphoreHandle_t s) { assert(locked); locked = false; s->mutex.unlock(); }
void portENTER_CRITICAL(portMUX_TYPE *) { assert(locked && !critical); critical = true; }
void portEXIT_CRITICAL(portMUX_TYPE *) { assert(critical); critical = false; }
uint32_t micros() { return nowUs; }
void delayMicroseconds(uint32_t us) {
    if (critical) assert(us < 500);
    nowUs += us;
}
void pinMode(int pin, int mode) { assert(pin == 21 && mode == OUTPUT); }
void digitalWrite(int pin, int level) {
    assert(pin == 21);
    assert(locked || !started); // Initialization runs before other tasks exist.
    ++writes;
    if (level == LOW && output == HIGH) {
        assert(uint32_t(nowUs - highAt) >= 2);
        lowAt = nowUs;
    }
    if (level == HIGH && output == LOW) {
        uint32_t lowTime = nowUs - lowAt;
        if (lowTime >= 3000) decodedStep = 16;
        else {
            assert(critical && lowTime >= 2 && lowTime < 500);
            decodedStep = decodedStep == 1 ? 16 : decodedStep - 1;
        }
        highAt = nowUs;
    }
    if (!started) lowAt = nowUs;
    output = level;
}
'''
        checks = r'''
void expect(uint8_t percent) {
    setPulseBacklight(percent);
    unsigned int expected = (unsigned(percent > 100 ? 100 : percent) * 16 + 99) / 100;
    assert(backlightStep == expected);
    assert(!locked && !critical);
    if (expected) assert(decodedStep == int(expected) && output == HIGH);
    else assert(output == LOW);
}
int main() {
    setupPulseBacklight();
    started = true;
    expect(1); // Enabling immediately must first finish shutdown, then send 15 pulses.
    assert(nowUs >= 3000 + 30 + 15 * 4);
    unsigned int beforeWrites = writes;
    uint32_t beforeTime = nowUs;
    expect(2); // Same quantized step must not touch the pin or wait.
    assert(writes == beforeWrites && nowUs == beforeTime);
    // Exhaust every pair of nonzero steps, including modulo wrap from dim to bright.
    for (unsigned from = 1; from <= 100; ++from) {
        for (unsigned to = 1; to <= 100; ++to) {
            expect(from);
            expect(to);
        }
    }
    expect(255); // Clamp over-range callers.
    expect(0);
    beforeTime = nowUs;
    expect(100); // Fast off/on must not be interpreted as one extra dimming pulse.
    assert(uint32_t(nowUs - beforeTime) >= 3030);
    nowUs = UINT32_MAX - 500;
    expect(0);
    nowUs += 1000; // micros() wraps while off.
    beforeTime = nowUs;
    expect(50);
    assert(uint32_t(nowUs - beforeTime) >= 2000 + 30);
    expect(0);
    nowUs += 10000;
    beforeTime = nowUs;
    expect(100); // Already settled: no repeated shutdown wait.
    assert(uint32_t(nowUs - beforeTime) == 30);
    // UI/settings and input-task fades must not interleave their pulse trains.
    auto writer = [](unsigned stride) {
        for (unsigned i = 0; i < 2000; ++i) setPulseBacklight((i * stride) % 101);
    };
    std::thread settings(writer, 7), fade(writer, 13);
    settings.join();
    fade.join();
    expect(100);
}
'''
        with tempfile.TemporaryDirectory(prefix="bruce-backlight-") as tmp:
            cpp = Path(tmp) / "test.cpp"
            exe = Path(tmp) / ("test.exe" if os.name == "nt" else "test")
            cpp.write_text(harness + driver + checks, encoding="utf-8")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            str(cpp), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_board_ownership(self):
        profile = (BOARD / "lilygo-t-embed-cc1101.ini").read_text(encoding="utf-8")
        other = (BOARD / "lilygo-t-embed.ini").read_text(encoding="utf-8")
        interface = (BOARD / "interface.cpp").read_text(encoding="utf-8")
        self.assertIn("-DT_EMBED_1101=1", profile)
        self.assertIn("-DTFT_RST=-1", profile)
        self.assertNotIn("-DTFT_RST=40", profile)
        self.assertIn("-DTFT_RST=9", other)
        self.assertNotIn("-DT_EMBED_1101", other)
        self.assertIn("#ifdef T_EMBED_1101\n    setPulseBacklight(100);\n#else", interface)
        self.assertIn("#ifdef T_EMBED_1101\n    setPulseBacklight(brightval);\n#else", interface)
        self.assertIn("hal_bright_set(TFT_BL, brightval);", interface)


if __name__ == "__main__":
    unittest.main()
