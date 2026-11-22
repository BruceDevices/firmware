import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ScrollerInputTest(unittest.TestCase):
    def test_turning_keeps_polling_and_pending_back_is_preserved(self):
        main = (ROOT / 'src/main.cpp').read_text()
        task = main[main.index('void __attribute__((weak)) taskInputHandler('):main.index('// Public Globals Variables')]
        encoder = (ROOT / 'src/hal/inputs/encoder.cpp').read_text()
        poll = encoder[encoder.index('void hal_encoder_poll('):encoder.index('#else\nvoid hal_encoder_init')]
        remote = (ROOT / 'src/modules/badusb_ble/ducky_typer.cpp').read_text()
        start = remote.index('        int32_t steps = 0;', remote.index('void MangaRemote('))
        consume = remote[start:remote.index('        // Bruce encodes UI Next', start)]
        stub = r'''
#include <cassert>
#include <cstdint>

static unsigned long tick = 1000;
unsigned long millis() { return tick; }
void checkPowerSaveTime() {}
void checkAndRecoverSysI2CBus() {}
void inputLock() {}
void inputUnlock() {}
bool wakeUpScreen() { return false; }
constexpr int LOW = 0;
int digitalRead(int) { return 1; }
struct DeviceEncoder { int pin_sel = 0, pin_esc = 6; };
struct TestEncoder { long getPosition() { return (millis() - 1000) / 10 + 1; } } encoder;
auto halEncoder = &encoder;
bool NextPress, PrevPress, UpPress, DownPress, SelPress, EscPress, AnyKeyPress, SerialCmdPress;
bool NextPagePress, PrevPagePress, LongPress;
int32_t RotaryNetSteps;
int32_t drainRotarySteps() { const auto steps = RotaryNetSteps; RotaryNetSteps = 0; return steps; }
struct { bool pressed; void Clear() {} } touchPoint;
void InputHandler();
#define pdMS_TO_TICKS(x) (x)
struct Done {};
int32_t consumeSteps() {
'''
        harness = r'''
    return steps;
}
void vTaskDelay(unsigned long amount) {
    assert(consumeSteps() == 1);
    tick += amount;
    if (tick >= 1100) throw Done{};
}
'''
        check = r'''
void InputHandler() { hal_encoder_poll(DeviceEncoder{}); }
int main() {
    try { taskInputHandler(nullptr); } catch (const Done &) {}
    assert(tick == 1100);

    AnyKeyPress = EscPress = true;
    RotaryNetSteps = 2;
    assert(consumeSteps() == 2 && EscPress && AnyKeyPress);

    EscPress = false;
    SerialCmdPress = true;
    RotaryNetSteps = -1;
    assert(consumeSteps() == -1 && SerialCmdPress && AnyKeyPress);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / 'input.cpp'
            binary = cpp.with_suffix('')
            cpp.write_text(stub + consume + harness + poll + task + check)
            subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', str(cpp),
                            '-o', str(binary)], check=True)
            subprocess.run([str(binary)], cwd=directory, check=True)
