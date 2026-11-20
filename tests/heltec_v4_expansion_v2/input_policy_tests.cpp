#include "boards/heltec-v4-expansion-v2/board_input.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using heltec_v4::Button;
using heltec_v4::ButtonEvent;

void require(bool condition, const char *expression, int line) {
    if (!condition) throw std::runtime_error(std::string("line ") + std::to_string(line) + ": " + expression);
}
#define REQUIRE(expression) require((expression), #expression, __LINE__)

std::array<uint8_t, 7> report(uint16_t x, uint16_t y, uint8_t count = 1, uint8_t event = 0) {
    return {{0, 0, count, uint8_t((event << 6) | (x >> 8)), uint8_t(x), uint8_t(y >> 8), uint8_t(y)}};
}

void expectRejected(const std::array<uint8_t, 7> &bytes, size_t size = 7) {
    uint16_t x = 1234, y = 2345;
    REQUIRE(!heltec_v4::decodeTouch(bytes.data(), size, x, y));
    REQUIRE(x == 1234 && y == 2345);
}

void touchReportValidation() {
    auto bytes = report(239, 319);
    for (size_t size = 0; size < bytes.size(); ++size) expectRejected(bytes, size);
    expectRejected(bytes, 8);
    for (uint8_t count : {uint8_t(0), uint8_t(3), uint8_t(4), uint8_t(5), uint8_t(6), uint8_t(7)}) {
        expectRejected(report(0, 0, count));
    }
    expectRejected(report(0, 0, 1, 1)); // Controller release event.
    expectRejected(report(0, 0, 1, 3)); // Reserved event.
    expectRejected(report(240, 0));
    expectRejected(report(0, 320));
    expectRejected(report(4095, 319));
    expectRejected(report(239, 4095));
    for (uint8_t count : {uint8_t(1), uint8_t(2)}) {
        for (uint8_t event : {uint8_t(0), uint8_t(2)}) {
            bytes = report(239, 319, count, event);
            bytes[2] |= 0xF8; // Other status bits are not the point count.
            bytes[3] |= 0x30;
            bytes[5] |= 0xF0; // Point-ID bits are not Y coordinate bits.
            uint16_t x = 0, y = 0;
            REQUIRE(heltec_v4::decodeTouch(bytes.data(), bytes.size(), x, y));
            REQUIRE(x == 239 && y == 319);
        }
    }
}

void everyNativePixelDecodes() {
    for (uint16_t rawY = 0; rawY < 320; ++rawY) {
        for (uint16_t rawX = 0; rawX < 240; ++rawX) {
            const auto bytes = report(rawX, rawY, 1, 2);
            uint16_t x = 65535, y = 65535;
            REQUIRE(heltec_v4::decodeTouch(bytes.data(), bytes.size(), x, y));
            REQUIRE(x == rawX && y == rawY);
        }
    }
}

void rotationCornersAndNavigationStrip() {
    // Expected coordinates are fixed panel landmarks, including the final row
    // reserved for Bruce's navigation strip. They do not use the production formula.
    struct Landmark { uint16_t rawX, rawY; uint16_t rotated[4][2]; };
    const Landmark points[] = {
        {0, 0, {{0, 0}, {0, 239}, {239, 319}, {319, 0}}},
        {239, 0, {{239, 0}, {0, 0}, {0, 319}, {319, 239}}},
        {0, 319, {{0, 319}, {319, 239}, {239, 0}, {0, 0}}},
        {239, 319, {{239, 319}, {319, 0}, {0, 0}, {0, 239}}},
        {37, 211, {{37, 211}, {211, 202}, {202, 108}, {108, 37}}},
        {219, 17, {{219, 17}, {17, 20}, {20, 302}, {302, 219}}},
        {220, 17, {{220, 17}, {17, 19}, {19, 302}, {302, 220}}},
    };
    for (const auto &point : points) {
        for (uint8_t rotation = 0; rotation < 8; ++rotation) {
            uint16_t x = point.rawX, y = point.rawY;
            heltec_v4::rotateTouch(rotation, x, y);
            REQUIRE(x == point.rotated[rotation % 4][0]);
            REQUIRE(y == point.rotated[rotation % 4][1]);
        }
    }
}

void everyRotationIsBoundedAndBijective() {
    for (uint8_t rotation = 0; rotation < 4; ++rotation) {
        const uint16_t width = (rotation & 1) ? 320 : 240;
        const uint16_t height = (rotation & 1) ? 240 : 320;
        std::vector<bool> visited(size_t(width) * height, false);
        for (uint16_t rawY = 0; rawY < 320; ++rawY) {
            for (uint16_t rawX = 0; rawX < 240; ++rawX) {
                uint16_t x = rawX, y = rawY;
                heltec_v4::rotateTouch(rotation, x, y);
                REQUIRE(x < width && y < height);
                const size_t pixel = size_t(y) * width + x;
                REQUIRE(!visited[pixel]);
                visited[pixel] = true;
            }
        }
        for (bool pixel : visited) REQUIRE(pixel);
    }
}

void quickTapAndBounce() {
    Button button;
    button.begin(false, 0);
    REQUIRE(button.update(true, 100) == ButtonEvent::None);
    REQUIRE(button.update(false, 110) == ButtonEvent::None); // Contact chatter.
    REQUIRE(button.update(true, 120) == ButtonEvent::None);
    REQUIRE(button.update(true, 144) == ButtonEvent::None);
    REQUIRE(button.update(true, 145) == ButtonEvent::None); // Debounced down.
    REQUIRE(button.update(false, 150) == ButtonEvent::None); // Quick, valid tap.
    REQUIRE(button.update(false, 174) == ButtonEvent::None);
    REQUIRE(button.update(false, 175) == ButtonEvent::Click);
    REQUIRE(button.update(false, 176) == ButtonEvent::None);
    REQUIRE(button.update(false, 2000) == ButtonEvent::None);

    button.begin(false, 0);
    REQUIRE(button.update(true, 10) == ButtonEvent::None);
    REQUIRE(button.update(false, 34) == ButtonEvent::None); // Below debounce duration.
    REQUIRE(button.update(false, 59) == ButtonEvent::None);
    REQUIRE(button.update(false, 1000) == ButtonEvent::None);
}

void releaseBounceProducesOneClick() {
    Button button;
    button.begin(false, 0);
    REQUIRE(button.update(true, 10) == ButtonEvent::None);
    REQUIRE(button.update(true, 35) == ButtonEvent::None);
    REQUIRE(button.update(false, 60) == ButtonEvent::None);
    REQUIRE(button.update(true, 70) == ButtonEvent::None);
    REQUIRE(button.update(false, 80) == ButtonEvent::None);
    REQUIRE(button.update(false, 104) == ButtonEvent::None);
    REQUIRE(button.update(false, 105) == ButtonEvent::Click);
    REQUIRE(button.update(false, 106) == ButtonEvent::None);
}

void holdIsOneEventAndSuppressesReleaseClick() {
    Button button;
    button.begin(false, 0);
    REQUIRE(button.update(true, 100) == ButtonEvent::None);
    REQUIRE(button.update(true, 125) == ButtonEvent::None);
    REQUIRE(button.update(true, 774) == ButtonEvent::None);
    REQUIRE(button.update(true, 775) == ButtonEvent::Hold);
    REQUIRE(button.update(true, 776) == ButtonEvent::None);
    REQUIRE(button.update(true, 9000) == ButtonEvent::None);
    REQUIRE(button.update(false, 9010) == ButtonEvent::None);
    REQUIRE(button.update(false, 9035) == ButtonEvent::None);
    REQUIRE(button.update(true, 9100) == ButtonEvent::None);
    REQUIRE(button.update(true, 9125) == ButtonEvent::None);
    REQUIRE(button.update(false, 9150) == ButtonEvent::None);
    REQUIRE(button.update(false, 9175) == ButtonEvent::Click);
}

void pendingReleaseCannotBecomeHold() {
    Button button;
    button.begin(false, 0);
    REQUIRE(button.update(true, 100) == ButtonEvent::None);
    REQUIRE(button.update(true, 125) == ButtonEvent::None);
    REQUIRE(button.update(false, 770) == ButtonEvent::None); // Released before hold deadline.
    REQUIRE(button.update(false, 775) == ButtonEvent::None); // Release still debouncing.
    REQUIRE(button.update(false, 794) == ButtonEvent::None);
    REQUIRE(button.update(false, 795) == ButtonEvent::Click);
    REQUIRE(button.update(false, 800) == ButtonEvent::None);
}

void bounceNearHoldDeadlinePreservesHold() {
    Button button;
    button.begin(false, 0);
    REQUIRE(button.update(true, 100) == ButtonEvent::None);
    REQUIRE(button.update(true, 125) == ButtonEvent::None);
    REQUIRE(button.update(false, 770) == ButtonEvent::None);
    REQUIRE(button.update(false, 775) == ButtonEvent::None);
    REQUIRE(button.update(true, 780) == ButtonEvent::Hold); // Release was only chatter.
    REQUIRE(button.update(true, 1000) == ButtonEvent::None);
    REQUIRE(button.update(false, 1010) == ButtonEvent::None);
    REQUIRE(button.update(false, 1035) == ButtonEvent::None);
}

void bootAndWakeHeldButtonAreSuppressed() {
    Button button;
    button.begin(true, 100);
    REQUIRE(button.update(true, 100) == ButtonEvent::None);
    REQUIRE(button.update(true, 5000) == ButtonEvent::None);
    REQUIRE(button.update(false, 5010) == ButtonEvent::None);
    REQUIRE(button.update(false, 5035) == ButtonEvent::None);
    REQUIRE(button.update(true, 5100) == ButtonEvent::None);
    REQUIRE(button.update(true, 5125) == ButtonEvent::None);
    REQUIRE(button.update(false, 5150) == ButtonEvent::None);
    REQUIRE(button.update(false, 5175) == ButtonEvent::Click);
    button.begin(true, 6000); // Reinitialization after wake clears the previous state.
    REQUIRE(button.update(true, 10000) == ButtonEvent::None);
    REQUIRE(button.update(false, 10010) == ButtonEvent::None);
    REQUIRE(button.update(false, 10035) == ButtonEvent::None);
}

void independentButtonsPreserveSimultaneousEvents() {
    Button user, next;
    user.begin(false, 0);
    next.begin(false, 0);
    REQUIRE(user.update(true, 100) == ButtonEvent::None);
    REQUIRE(next.update(true, 100) == ButtonEvent::None);
    REQUIRE(user.update(true, 125) == ButtonEvent::None);
    REQUIRE(next.update(true, 125) == ButtonEvent::None);
    REQUIRE(next.update(false, 150) == ButtonEvent::None);
    REQUIRE(next.update(false, 175) == ButtonEvent::Click);
    REQUIRE(user.update(true, 775) == ButtonEvent::Hold);
    REQUIRE(next.update(false, 775) == ButtonEvent::None);
    REQUIRE(user.update(false, 800) == ButtonEvent::None);
    REQUIRE(user.update(false, 825) == ButtonEvent::None);
}

void debounceAndHoldSurviveMillisWrap() {
    const uint32_t start = UINT32_MAX - 20;
    Button button;
    button.begin(false, start - 10);
    REQUIRE(button.update(true, start) == ButtonEvent::None);
    REQUIRE(button.update(true, start + 24) == ButtonEvent::None);
    REQUIRE(button.update(true, start + 25) == ButtonEvent::None);
    REQUIRE(button.update(true, start + 674) == ButtonEvent::None);
    REQUIRE(button.update(true, start + 675) == ButtonEvent::Hold);
    REQUIRE(button.update(false, start + 700) == ButtonEvent::None);
    REQUIRE(button.update(false, start + 725) == ButtonEvent::None);

    button.begin(false, start - 100);
    REQUIRE(button.update(true, start - 75) == ButtonEvent::None);
    REQUIRE(button.update(true, start - 50) == ButtonEvent::None);
    REQUIRE(button.update(false, start) == ButtonEvent::None);
    REQUIRE(button.update(false, start + 24) == ButtonEvent::None);
    REQUIRE(button.update(false, start + 25) == ButtonEvent::Click);
}

void inputQueueCapacityAndOverflowPreserveOldest() {
    heltec_v4::InputQueue queue;
    heltec_v4::InputEvent event{heltec_v4::Back, 1234, 2345};
    REQUIRE(!queue.pop(event));
    REQUIRE(event.actions == heltec_v4::Back && event.x == 1234 && event.y == 2345);
    for (uint16_t index = 0; index < 8; ++index) {
        REQUIRE(queue.push({heltec_v4::Touch, index, uint16_t(index + 100)}));
    }
    REQUIRE(!queue.push({heltec_v4::Select, 999, 999})); // Drop newest, not oldest.
    for (uint16_t index = 0; index < 8; ++index) {
        REQUIRE(queue.pop(event));
        REQUIRE(event.actions == heltec_v4::Touch);
        REQUIRE(event.x == index && event.y == index + 100);
    }
    REQUIRE(!queue.pop(event));
    REQUIRE(queue.push({uint8_t(heltec_v4::Select | heltec_v4::Next), 0, 0}));
    REQUIRE(queue.pop(event));
    REQUIRE(event.actions == (heltec_v4::Select | heltec_v4::Next));
}

void inputQueueWraparoundRetainsFifoOrder() {
    heltec_v4::InputQueue queue;
    for (uint16_t index = 0; index < 8; ++index) {
        REQUIRE(queue.push({heltec_v4::Touch, index, uint16_t(index + 100)}));
    }
    // Keep the queue full while its head wraps many times; inserted events
    // must never replace records waiting for the UI consumer.
    heltec_v4::InputEvent event;
    for (uint16_t index = 0; index < 1024; ++index) {
        REQUIRE(queue.pop(event));
        REQUIRE(event.actions == heltec_v4::Touch);
        REQUIRE(event.x == index && event.y == index + 100);
        REQUIRE(queue.push({heltec_v4::Touch, uint16_t(index + 8), uint16_t(index + 108)}));
    }
    for (uint16_t index = 1024; index < 1032; ++index) {
        REQUIRE(queue.pop(event));
        REQUIRE(event.x == index && event.y == index + 100);
    }
    REQUIRE(!queue.pop(event));
}

void rapidTouchesWaitForUiConsumption() {
    heltec_v4::InputQueue queue;
    // The first event is consumed; the UI is still handling it during the
    // next two touch edges. Sampling queues both without waiting for the UI.
    REQUIRE(queue.push({heltec_v4::Select, 0, 0}));
    heltec_v4::InputEvent event;
    REQUIRE(queue.pop(event));
    const auto first = report(239, 10);
    const auto second = report(0, 310);
    uint16_t x = 0, y = 0;
    REQUIRE(heltec_v4::decodeTouch(first.data(), first.size(), x, y));
    heltec_v4::rotateTouch(1, x, y);
    REQUIRE(queue.push({heltec_v4::Touch, x, y}));
    REQUIRE(heltec_v4::decodeTouch(second.data(), second.size(), x, y));
    heltec_v4::rotateTouch(1, x, y);
    REQUIRE(queue.push({heltec_v4::Touch, x, y}));
    REQUIRE(queue.push({heltec_v4::Back, 0, 0}));

    REQUIRE(queue.pop(event));
    REQUIRE(event.actions == heltec_v4::Touch && event.x == 10 && event.y == 0);
    REQUIRE(queue.pop(event));
    REQUIRE(event.actions == heltec_v4::Touch && event.x == 310 && event.y == 239);
    REQUIRE(queue.pop(event));
    REQUIRE(event.actions == heltec_v4::Back);
    REQUIRE(!queue.pop(event));
}
} // namespace

int main() {
    struct Test { const char *name; void (*run)(); };
    const Test tests[] = {
        {"CHSC report validation", touchReportValidation},
        {"all 76,800 native touch pixels", everyNativePixelDecodes},
        {"rotation corners and navigation strip", rotationCornersAndNavigationStrip},
        {"all rotations bounded and bijective", everyRotationIsBoundedAndBijective},
        {"quick tap and contact bounce", quickTapAndBounce},
        {"release bounce produces one click", releaseBounceProducesOneClick},
        {"hold is one event without a release click", holdIsOneEventAndSuppressesReleaseClick},
        {"pending release cannot become hold", pendingReleaseCannotBecomeHold},
        {"chatter near hold deadline", bounceNearHoldDeadlinePreservesHold},
        {"boot and wake held buttons suppressed", bootAndWakeHeldButtonAreSuppressed},
        {"independent simultaneous buttons", independentButtonsPreserveSimultaneousEvents},
        {"millis wrap during debounce and hold", debounceAndHoldSurviveMillisWrap},
        {"input queue capacity and overflow policy", inputQueueCapacityAndOverflowPreserveOldest},
        {"input queue FIFO wraparound", inputQueueWraparoundRetainsFifoOrder},
        {"rapid touches survive deferred UI consumption", rapidTouchesWaitForUiConsumption},
    };
    size_t passed = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            ++passed;
            std::cout << "PASS " << test.name << '\n';
        } catch (const std::exception &error) {
            std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    std::cout << passed << " Heltec input-policy tests passed.\n";
    return EXIT_SUCCESS;
}
