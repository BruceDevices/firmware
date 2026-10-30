#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Hardware-independent EMV parsing. No card data is logged or saved here.
namespace emv {
using Bytes = std::vector<uint8_t>;
using Exchange = std::function<bool(const Bytes &, Bytes &)>;
struct Card {
    Bytes aid;
    std::string label, pan, holder, validFrom, validTo;
};
struct Terminal {
    Bytes date = {0, 0, 0}; // YYMMDD, supplied from the device clock
    Bytes unpredictable = {0, 0, 0, 0};
};

Bytes pn532Command(const Bytes &command);
bool pn532Response(const Bytes &frame, uint8_t command, Bytes &payload);
bool readCard(const Exchange &exchange, const Terminal &terminal, Card &card, std::string &error);
} // namespace emv
