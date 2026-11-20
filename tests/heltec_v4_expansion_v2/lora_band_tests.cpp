// Compile both ways: the Heltec's physical frontend limits and generic boards.
#ifdef TEST_BOARD_LIMITS
#define LORA_MIN_FREQ_MHZ 863.0
#define LORA_MAX_FREQ_MHZ 928.0
#define LORA_DEFAULT_FREQ_HZ "915000000.00"
#endif
#include "src/modules/lora/lora_board.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char *expression, int line) {
    if (!condition) throw std::runtime_error(std::string("line ") + std::to_string(line) + ": " + expression);
}
#define REQUIRE(expression) require((expression), #expression, __LINE__)

void rejectNonfiniteAndInvalidBandwidth() {
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (double frequency : {infinity, -infinity, nan}) {
        REQUIRE(!loraFrequencySupported(frequency, 31.25));
    }
    for (double bandwidth : {0.0, -0.0, -1.0, -31.25, infinity, -infinity, nan}) {
        REQUIRE(!loraFrequencySupported(915.0, bandwidth));
    }
    REQUIRE(!loraFrequencySupported(0.0, 31.25));
    REQUIRE(!loraFrequencySupported(-915.0, 31.25));
    REQUIRE(!loraFrequencySupported(std::numeric_limits<double>::max(), 31.25));
    REQUIRE(!loraFrequencySupported(std::numeric_limits<double>::lowest(), 31.25));
}

#ifdef TEST_BOARD_LIMITS
void entireChannelFitsAtBandEdges() {
    // 31.25 kHz has exactly representable half-bandwidth: 0.015625 MHz.
    constexpr double lowerCenter = 863.015625;
    constexpr double upperCenter = 927.984375;
    REQUIRE(loraFrequencySupported(lowerCenter, 31.25));
    REQUIRE(loraFrequencySupported(upperCenter, 31.25));
    REQUIRE(!loraFrequencySupported(std::nextafter(lowerCenter, 0.0), 31.25));
    REQUIRE(!loraFrequencySupported(std::nextafter(upperCenter, 1000.0), 31.25));
    REQUIRE(loraFrequencySupported(std::nextafter(lowerCenter, 1000.0), 31.25));
    REQUIRE(loraFrequencySupported(std::nextafter(upperCenter, 0.0), 31.25));
    REQUIRE(!loraFrequencySupported(863.0, 31.25));
    REQUIRE(!loraFrequencySupported(928.0, 31.25));
    REQUIRE(!loraFrequencySupported(862.99, 31.25));
    REQUIRE(!loraFrequencySupported(928.01, 31.25));

    REQUIRE(loraFrequencySupported(863.0625, 125.0));
    REQUIRE(loraFrequencySupported(927.9375, 125.0));
    REQUIRE(!loraFrequencySupported(863.0624, 125.0));
    REQUIRE(!loraFrequencySupported(927.9376, 125.0));
    REQUIRE(loraFrequencySupported(863.25, 500.0));
    REQUIRE(loraFrequencySupported(927.75, 500.0));
    REQUIRE(!loraFrequencySupported(863.249, 500.0));
    REQUIRE(!loraFrequencySupported(927.751, 500.0));
}

void boardRejectsImportedLowBandConfiguration() {
    REQUIRE(!loraFrequencySupported(434.5, 31.25));
    REQUIRE(!loraFrequencySupported(433.92, 31.25));
    REQUIRE(!loraFrequencySupported(434500000.0, 31.25)); // Wrong units also fail closed.
    REQUIRE(loraFrequencySupported(868.0, 31.25));
    REQUIRE(loraFrequencySupported(915.0, 31.25));
    REQUIRE(loraFrequencySupported(923.0, 31.25));
    REQUIRE(std::string(LORA_DEFAULT_FREQ_HZ) == "915000000.00");
    REQUIRE(loraFrequencySupported(std::stod(LORA_DEFAULT_FREQ_HZ) / 1000000.0, 31.25));
}

void channelCannotExceedWholePhysicalBand() {
    // This helper checks occupied bandwidth, not whether the radio supports a
    // particular modulation bandwidth. RadioLib still performs that validation.
    REQUIRE(loraFrequencySupported(895.5, 65000.0));
    REQUIRE(!loraFrequencySupported(895.5, 65000.1));
    REQUIRE(!loraFrequencySupported(915.0, 65000.0));
    REQUIRE(!loraFrequencySupported(915.0, std::numeric_limits<double>::max()));
}
#else
void genericFrequencyRangeRemainsAvailable() {
    REQUIRE(loraFrequencySupported(434.5, 31.25));
    REQUIRE(loraFrequencySupported(433.92, 125.0));
    REQUIRE(loraFrequencySupported(868.0, 31.25));
    REQUIRE(loraFrequencySupported(915.0, 31.25));
    REQUIRE(loraFrequencySupported(1000.0, 31.25));
    REQUIRE(!loraFrequencySupported(std::nextafter(1000.0, 2000.0), 31.25));
    REQUIRE(loraFrequencySupported(0.001, 31.25));
    // A generic board has no frontend band boundaries. Its transceiver driver
    // retains responsibility for enforcing the radio's supported bandwidth.
    REQUIRE(loraFrequencySupported(434.5, 65000.0));
    REQUIRE(std::string(LORA_DEFAULT_FREQ_HZ) == "434500000.00");
    REQUIRE(loraFrequencySupported(std::stod(LORA_DEFAULT_FREQ_HZ) / 1000000.0, 31.25));
}
#endif
} // namespace

int main() {
    try {
        rejectNonfiniteAndInvalidBandwidth();
#ifdef TEST_BOARD_LIMITS
        entireChannelFitsAtBandEdges();
        boardRejectsImportedLowBandConfiguration();
        channelCannotExceedWholePhysicalBand();
        std::cout << "PASS Heltec LoRa occupied-band boundaries, imported settings, and invalid values\n";
#else
        genericFrequencyRangeRemainsAvailable();
        std::cout << "PASS generic LoRa defaults, range, and invalid values\n";
#endif
    } catch (const std::exception &error) {
        std::cerr << "FAIL LoRa board policy: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
