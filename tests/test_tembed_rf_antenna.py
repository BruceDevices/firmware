#!/usr/bin/env python3
"""Host regression test of the actual antenna-switch block in rf_utils.cpp.
Run: python tests/test_tembed_rf_antenna.py [path/to/rf_utils.cpp]
Requires g++; no ESP32 hardware or PlatformIO dependencies are needed.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'src/modules/rf/rf_utils.cpp'
text = source.read_text().split('void setMHZ(float frequency) {', 1)[1]
lines = text[text.index('#if defined(T_EMBED)'):].splitlines(keepends=True)
depth = 0
block = ''
for line in lines:
    directive = line.strip()
    if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
        depth += 1
    elif directive.startswith('#endif'):
        depth -= 1
    block += line
    if depth == 0:
        break
# Compile the unchanged production block under each board's macro environment.
preamble = r'''
#include <cstdint>
#include <cstdio>
#define HIGH 1
#define LOW 0
#define CC1101_SW1_PIN 47
#define CC1101_SW0_PIN 48
#define portTICK_PERIOD_MS 1
struct { struct { int cs; } CC1101_bus; } bruceConfigPins;
int pins[49] = {};
int writes = 0;
int delays = 0;
void digitalWrite(int pin, int state) { pins[pin] = state; ++writes; }
void vTaskDelay(int ticks) { if (ticks != 10) __builtin_abort(); ++delays; }
void tune(float frequency) {
'''
main = r'''
}
int main() {
    bruceConfigPins.CC1101_bus.cs = CHIP_SELECT;
    struct Case { float frequency; int sw1; int sw0; } cases[] = {
        {433.92f, 1, 1}, {315.0f, 1, 0}, {433.92f, 1, 1},
        {868.35f, 0, 1}, {915.0f, 0, 1}, {433.92f, 1, 1}
    };
    for (auto c : cases) {
        tune(c.frequency);
        if (EXPECT_SWITCH && (pins[47] != c.sw1 || pins[48] != c.sw0)) {
            std::fprintf(stderr, "wrong antenna path at %.2f MHz: SW1=%d SW0=%d\n",
                         c.frequency, pins[47], pins[48]);
            return 1;
        }
    }
    if (EXPECT_SWITCH) return (writes == 10 && delays == 5) ? 0 : 2;
    return (writes == 0 && delays == 0) ? 0 : 3;
}
'''
variants = [
    ('CC1101 beta board', ['-DT_EMBED_1101=1', '-DCHIP_SELECT=12', '-DEXPECT_SWITCH=1']),
    ('legacy CC1101 board', ['-DT_EMBED=1', '-DT_EMBED_1101=1', '-DCHIP_SELECT=12', '-DEXPECT_SWITCH=1']),
    ('legacy switched module', ['-DT_EMBED=1', '-DCHIP_SELECT=17', '-DEXPECT_SWITCH=1']),
    ('legacy external module', ['-DT_EMBED=1', '-DCHIP_SELECT=43', '-DEXPECT_SWITCH=0']),
    ('unrelated board', ['-DCHIP_SELECT=12', '-DEXPECT_SWITCH=0']),
]
with tempfile.TemporaryDirectory() as d:
    src, exe = Path(d) / 'test.cpp', Path(d) / 'test'
    src.write_text(preamble + block + main)
    for name, flags in variants:
        subprocess.run(['g++', '-std=c++11', *flags, str(src), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
        print('PASS:', name)
