// Host regression: compile with -std=c++17 -Iinclude. No Arduino toolchain or
// board access. The fade test includes the production implementation directly.
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include "../src/hal/inputs/encoder_state.h"
#include "../src/core/remote_select.h"

// Exclude hardware headers while compiling the real power-save state machine.
#define __DISPLAY_H__
#define __SETTINGS_H__
#define __GLOBALS__
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ 240
using std::max;
using std::min;

struct { int dimmerSet = 1; int bright = 100; } bruceConfig;
std::atomic<unsigned long> previousMillis{0};
RemoteSelectState remoteSelectState;
volatile bool SelPress = false, SerialCmdPress = false, AnyKeyPress = false;
bool dimmer = false, isSleeping = false, isScreenOff = false;
uint32_t nowMs = 0;
std::vector<int> brightnessWrites;
unsigned delays = 0;
std::recursive_mutex inputMutex;
unsigned long millis() { return nowMs; }
void inputLock() { inputMutex.lock(); }
void inputUnlock() { inputMutex.unlock(); }
void setBrightness(uint8_t brightness, bool) { brightnessWrites.push_back(brightness); }
void turnOffDisplay() { setBrightness(0, false); }
void delay(unsigned) { ++delays; }
void setCpuFrequencyMhz(int) {}
void panelSleep(bool) {}
void disableCore0WDT() {}
void disableLoopWDT() {}
void enableCore0WDT() {}
void enableLoopWDT() {}
void feedLoopWDT() {}
void getBrightness() {}
#include "../src/core/powerSave.cpp"

static EncoderButtonState press(uint32_t generation, uint32_t pressedAt) {
    EncoderButtonState state;
    state.down = true;
    state.generation = generation;
    state.pressedAt = pressedAt;
    return state;
}

static void testDebounceAndShortClick() {
    EncoderButtonDebounce button;
    button.init(false, 0);
    assert(!button.sample(true, 10));
    assert(!button.sample(false, 20));
    assert(!button.sample(true, 30));
    assert(!button.sample(true, 49));
    assert(button.sample(true, 50));
    assert(button.state.generation == 1 && button.state.pressedAt == 30);
    assert(!button.sample(false, 60));
    assert(button.sample(false, 80));
    // Both edges happened while a legacy consumer could be stalled for 75ms.
    EncoderMenuSelect menu(0);
    bool held = true;
    assert(menu.update(button.state, 100, 0, false, true, held));
    assert(!held);
    assert(!menu.update(button.state, 110, 0, false, true, held));
}

static void testMenuHoldAndReentry() {
    bool held;
    auto button = press(1, 100);
    EncoderMenuSelect child(1);
    assert(!child.update(button, 900, 0, false, false, held));
    button.down = false;
    button.releasedAt = 910;
    assert(!child.update(button, 930, 0, false, false, held));
    button = press(2, 940);
    assert(child.update(button, 960, 0, false, false, held));

    EncoderMenuSelect menu(0);
    button = press(1, 100);
    assert(!menu.update(button, 120, 2, false, true, held));
    assert(!menu.update(button, 599, 2, false, true, held));
    assert(menu.update(button, 600, 2, false, true, held) && held);
    assert(!menu.update(button, 800, 2, false, true, held));

    // Release duration, not the time when a slow redraw observes it, decides.
    EncoderMenuSelect released(0);
    button.down = false;
    button.releasedAt = 599;
    assert(released.update(button, 900, 2, false, true, held) && !held);
    EncoderMenuSelect threshold(0);
    button.releasedAt = 600;
    assert(threshold.update(button, 900, 2, false, true, held) && held);
}

static void testWakeMoveAndRollover() {
    bool held;
    auto button = press(1, 100);
    button.suppressed = true;
    EncoderMenuSelect menu(0);
    assert(!menu.update(button, 120, 0, false, false, held));
    button.down = false;
    button.releasedAt = 150;
    assert(!menu.update(button, 170, 0, false, false, held));
    button = press(2, 200);
    assert(!menu.update(button, 220, 0, false, true, held));
    assert(!menu.update(button, 300, 1, true, true, held));
    assert(!menu.update(button, 800, 1, false, true, held));
    // A fresh click accompanying a drained rotation selects the updated row;
    // only an already pending hold is cancelled by navigation.
    EncoderMenuSelect rotated(0);
    auto rotatedButton = press(1, 810);
    assert(rotated.update(rotatedButton, 830, 2, true, false, held));
    EncoderMenuSelect rotatedHold(0);
    assert(!rotatedHold.update(rotatedButton, 830, 2, true, true, held));
    rotatedButton.down = false;
    rotatedButton.releasedAt = 850;
    assert(rotatedHold.update(rotatedButton, 870, 2, false, true, held) && !held);
    button = press(3, 900);
    assert(!menu.update(button, 920, 1, false, true, held));
    menu.cancel(); // Synthetic selection or abandoning this menu cancels hold.
    assert(!menu.update(button, 1500, 1, false, true, held));

    EncoderMenuSelect wrapped(UINT32_MAX);
    button = press(0, UINT32_MAX - 100);
    assert(!wrapped.update(button, UINT32_MAX - 80, 0, false, true, held));
    assert(wrapped.update(button, 399, 0, false, true, held) && held);
    EncoderButtonDebounce debounce;
    debounce.init(false, UINT32_MAX - 30);
    assert(!debounce.sample(true, UINT32_MAX - 10));
    assert(debounce.sample(true, 9));
}

static void testProductionIdleFade() {
    auto tick = [](uint32_t time) { nowMs = time; checkPowerSaveTime(); };
    tick(999);
    assert(brightnessWrites.empty());
    tick(1000);
    assert(brightnessWrites == std::vector<int>{33});
    tick(5999);
    assert(brightnessWrites.size() == 1);
    tick(6000);
    const auto firstFadeWrites = brightnessWrites.size();
    tick(6001);
    assert(brightnessWrites.size() == firstFadeWrites);
    tick(6010);
    assert(brightnessWrites.back() == 31);
    tick(7000);
    assert(brightnessWrites.back() == 0 && isScreenOff);
    const auto completedWrites = brightnessWrites.size();
    tick(8000);
    assert(brightnessWrites.size() == completedWrites && delays == 0);

    // wakeUpScreen serializes these state changes with checkPowerSaveTime.
    inputLock();
    previousMillis = 8000;
    dimmer = isScreenOff = false;
    setBrightness(100, false);
    inputUnlock();
    tick(8001);
    assert(brightnessWrites.back() == 100 && !isScreenOff);
    tick(9000);
    tick(14000);
    const auto beforeDisabled = brightnessWrites.size();
    bruceConfig.dimmerSet = 0;
    tick(15000);
    assert(brightnessWrites.size() == beforeDisabled);

    bruceConfig.dimmerSet = 1;
    previousMillis = UINT32_MAX - 500;
    dimmer = isScreenOff = false;
    brightnessWrites.clear();
    tick(498);
    assert(brightnessWrites.empty());
    tick(499);
    assert(brightnessWrites == std::vector<int>{33});
    tick(5499);
    tick(5664);
    assert(brightnessWrites.back() == 0 && delays == 0);
}

static void testRemotePublicationAndRepeats() {
    RemoteMenuSelect browser(readRemoteSelectState().generation);
    uint32_t webCommand = 0;
    publishRemoteSelect(500, webCommand); // /cm nav sel 500
    bool held = false;
    assert(SelPress && SerialCmdPress && AnyKeyPress);
    assert(browser.consume(readRemoteSelectState(), held) && held);

    RemoteMenuSelect context(readRemoteSelectState().generation);
    // Input reset may clear the compatibility pulse, but not publish a new ID.
    SelPress = SerialCmdPress = AnyKeyPress = false;
    remoteSelectState.legacyEvent = false;
    publishRemoteSelect(500, webCommand);
    assert(SelPress && !context.consume(readRemoteSelectState(), held));
    assert(!browser.consume(readRemoteSelectState(), held));

    uint32_t plainSerial = 0;
    publishRemoteSelect(1, plainSerial);
    assert(context.consume(readRemoteSelectState(), held) && !held);
    const auto latest = readRemoteSelectState();
    SelPress = SerialCmdPress = AnyKeyPress = false;
    publishRemoteSelect(500, webCommand); // Superseded callback cannot publish.
    assert(!SelPress && !SerialCmdPress && !AnyKeyPress);
    assert(readRemoteSelectState().generation == latest.generation);

    RemoteMenuSelect beforePublication(readRemoteSelectState().generation);
    uint32_t nextCommand = 0;
    publishRemoteSelect(499, nextCommand);
    assert(beforePublication.consume(readRemoteSelectState(), held) && !held);
    RemoteMenuSelect enteredAfterPublication(readRemoteSelectState().generation);
    assert(!enteredAfterPublication.consume(readRemoteSelectState(), held));
    remoteSelectState.generation = UINT32_MAX;
    uint32_t wrappedCommand = 0;
    publishRemoteSelect(500, wrappedCommand);
    assert(wrappedCommand == 1 && remoteSelectState.longPress);
}

int main() {
    testDebounceAndShortClick();
    testMenuHoldAndReentry();
    testWakeMoveAndRollover();
    testProductionIdleFade();
    testRemotePublicationAndRepeats();
    std::cout << "PASS encoder/menu events, production idle fade, remote duration/publication/repeats\n";
}
