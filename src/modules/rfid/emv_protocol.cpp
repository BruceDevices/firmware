#include "emv_protocol.h"
#include <algorithm>
#include <cstdio>

namespace emv {
namespace {
struct Tlv {
    uint32_t tag;
    Bytes value;
    std::vector<Tlv> children;
};

bool tagAt(const Bytes &data, size_t &pos, uint32_t &tag) {
    if (pos >= data.size()) return false;
    tag = data[pos++];
    if ((tag & 0x1f) != 0x1f) return true;
    for (unsigned i = 0; i < 2; ++i) {
        if (pos >= data.size()) return false;
        uint8_t next = data[pos++];
        tag = (tag << 8) | next;
        if (!(next & 0x80)) return true;
    }
    return false;
}

bool parse(const Bytes &data, std::vector<Tlv> &out, unsigned depth = 0) {
    if (depth > 8 || data.size() > 4096) return false;
    size_t pos = 0;
    while (pos < data.size()) {
        if (data[pos] == 0 || data[pos] == 0xff) {
            ++pos;
            continue;
        }
        const bool constructed = (data[pos] & 0x20) != 0;
        Tlv node{};
        if (!tagAt(data, pos, node.tag) || pos == data.size()) return false;
        size_t len = data[pos++];
        if (len & 0x80) {
            const size_t count = len & 0x7f;
            if (!count || count > 2 || count > data.size() - pos) return false;
            len = 0;
            for (size_t i = 0; i < count; ++i) len = (len << 8) | data[pos++];
        }
        if (len > data.size() - pos) return false;
        node.value.assign(data.begin() + pos, data.begin() + pos + len);
        pos += len;
        if (constructed && !parse(node.value, node.children, depth + 1)) return false;
        out.push_back(std::move(node));
    }
    return true;
}

const Tlv *find(const std::vector<Tlv> &nodes, uint32_t tag) {
    for (const auto &node : nodes) {
        if (node.tag == tag) return &node;
        if (const auto *found = find(node.children, tag)) return found;
    }
    return nullptr;
}

std::string printable(const Bytes &data) {
    std::string result;
    for (auto c : data)
        if (c >= 32 && c <= 126 && result.size() < 32) result += char(c);
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

std::string digits(const Bytes &data) {
    std::string result;
    bool padding = false;
    for (auto byte : data)
        for (unsigned shift : {4u, 0u}) {
            auto n = (byte >> shift) & 15;
            if (n == 15) padding = true;
            else if (n > 9 || padding) return {};
            else result += char('0' + n);
        }
    return result;
}

std::string date(const Bytes &data) {
    if (data.size() != 3) return {};
    auto d = digits(data);
    if (d.size() != 6 || d.substr(2, 2) < "01" || d.substr(2, 2) > "12") return {};
    return d.substr(2, 2) + "/" + d.substr(0, 2);
}

void fields(const std::vector<Tlv> &nodes, Card &card) {
    if (const auto *n = find(nodes, 0x50)) card.label = printable(n->value);
    if (const auto *n = find(nodes, 0x5f20)) card.holder = printable(n->value);
    if (const auto *n = find(nodes, 0x5f24)) card.validTo = date(n->value);
    if (const auto *n = find(nodes, 0x5f25)) card.validFrom = date(n->value);
    if (const auto *n = find(nodes, 0x5a)) {
        auto pan = digits(n->value);
        if (pan.size() >= 8 && pan.size() <= 19) card.pan = pan;
    }
    const auto *track = find(nodes, 0x57);
    if (!track) track = find(nodes, 0x9f6b);
    if (track) {
        std::string nibbles;
        for (auto b : track->value) {
            nibbles += "0123456789ABCDEF"[b >> 4];
            nibbles += "0123456789ABCDEF"[b & 15];
        }
        auto separator = nibbles.find('D');
        if (separator >= 8 && separator <= 19 && nibbles.size() >= separator + 5) {
            auto pan = nibbles.substr(0, separator);
            auto expiry = nibbles.substr(separator + 1, 4);
            if (pan.find_first_not_of("0123456789") == std::string::npos &&
                expiry.find_first_not_of("0123456789") == std::string::npos && expiry.substr(2, 2) >= "01" &&
                expiry.substr(2, 2) <= "12") {
                if (card.pan.empty()) card.pan = pan;
                if (card.validTo.empty()) card.validTo = expiry.substr(2, 2) + "/" + expiry.substr(0, 2);
            }
        }
    }
}

struct App {
    Bytes aid;
    std::string label;
    unsigned priority;
};
void applications(const std::vector<Tlv> &nodes, std::vector<App> &apps) {
    for (const auto &n : nodes) {
        if (n.tag == 0x61 && apps.size() < 16) {
            const auto *aid = find(n.children, 0x4f);
            if (!aid || aid->value.size() < 5 || aid->value.size() > 16) continue;
            if (std::any_of(apps.begin(), apps.end(), [&](const App &a) { return a.aid == aid->value; }))
                continue;
            const auto *label = find(n.children, 0x50);
            const auto *priority = find(n.children, 0x87);
            unsigned p = priority && priority->value.size() == 1 ? priority->value[0] & 15 : 0;
            apps.push_back({aid->value, label ? printable(label->value) : "", p ? p : 16});
        } else applications(n.children, apps);
    }
}

bool apdu(const Exchange &exchange, Bytes command, Bytes &data, std::string &error) {
    data.clear();
    for (unsigned step = 0; step < 8; ++step) {
        Bytes response;
        if (!exchange(command, response)) {
            error = "Card removed / NFC I/O error";
            return false;
        }
        if (response.size() < 2) {
            error = "Short card response";
            return false;
        }
        auto sw1 = response[response.size() - 2], sw2 = response.back();
        if (sw1 == 0x6c) {
            command.back() = sw2;
            continue;
        }
        if (data.size() + response.size() - 2 > 4096) break;
        data.insert(data.end(), response.begin(), response.end() - 2);
        if (sw1 == 0x90 && sw2 == 0) return true;
        if (sw1 == 0x61) {
            command = {0x00, 0xc0, 0x00, 0x00, sw2};
            continue;
        }
        char status[32];
        std::snprintf(status, sizeof(status), "Card status %02X%02X", sw1, sw2);
        error = status;
        return false;
    }
    error = "Card response limit exceeded";
    return false;
}

bool gpoCommand(const Bytes &pdol, const Terminal &terminal, Bytes &command) {
    Bytes values;
    size_t pos = 0;
    while (pos < pdol.size()) {
        uint32_t tag;
        if (!tagAt(pdol, pos, tag) || pos == pdol.size()) return false;
        size_t len = pdol[pos++];
        if (values.size() + len > 240) return false;
        Bytes value;
        switch (tag) {
            case 0x9f66: value = {0x79, 0x00, 0x40, 0x80}; break;
            case 0x9f59: value = {0xc8, 0x80, 0x00}; break;
            case 0x9f58: value = {0x01}; break;
            case 0x9f1a: value = {0x01, 0x24}; break; // Reader defaults, matching the reference
            case 0x5f2a: value = {0x01, 0x24}; break;
            case 0x9a: value = terminal.date; break;
            case 0x9f37: value = terminal.unpredictable; break;
            default: break; // Unknown DOL entries and amounts are zero-filled.
        }
        value.resize(len, 0);
        values.insert(values.end(), value.begin(), value.end());
    }
    Bytes object = {0x83};
    if (values.size() >= 128) object.push_back(0x81);
    object.push_back(static_cast<uint8_t>(values.size()));
    object.insert(object.end(), values.begin(), values.end());
    command = {0x80, 0xa8, 0, 0, static_cast<uint8_t>(object.size())};
    command.insert(command.end(), object.begin(), object.end());
    command.push_back(0);
    return true;
}
} // namespace

Bytes pn532Command(const Bytes &command) {
    if (command.empty() || command.size() > 253) return {};
    uint8_t len = command.size() + 1;
    Bytes frame = {0, 0, 0xff, len, static_cast<uint8_t>(-len), 0xd4};
    frame.insert(frame.end(), command.begin(), command.end());
    uint8_t sum = 0xd4;
    for (auto b : command) sum += b;
    frame.push_back(static_cast<uint8_t>(-sum));
    frame.push_back(0);
    return frame;
}

bool pn532Response(const Bytes &frame, uint8_t command, Bytes &payload) {
    payload.clear();
    if (frame.size() < 9 || frame[0] || frame[1] || frame[2] != 0xff) return false;
    size_t start = 5, len = frame[3];
    if (frame[3] == 0xff && frame[4] == 0xff) {
        if (frame.size() < 12 || uint8_t(frame[5] + frame[6] + frame[7]) != 0) return false;
        start = 8;
        len = (size_t(frame[5]) << 8) | frame[6];
    } else if (uint8_t(frame[3] + frame[4]) != 0) return false;
    if (len < 2 || len > frame.size() - start - 2 || frame[start] != 0xd5 ||
        frame[start + 1] != uint8_t(command + 1) || frame[start + len + 1] != 0)
        return false;
    uint8_t sum = 0;
    for (size_t i = start; i <= start + len; ++i) sum += frame[i];
    if (sum) return false;
    payload.assign(frame.begin() + start + 2, frame.begin() + start + len);
    return true;
}

bool readCard(const Exchange &exchange, const Terminal &terminal, Card &card, std::string &error) {
    card = Card{};
    error.clear();
    bool ioFailed = false;
    const Exchange transfer = [&](const Bytes &command, Bytes &data) {
        bool ok = exchange(command, data);
        if (!ok) ioFailed = true;
        return ok;
    };
    Bytes response;
    const Bytes ppse = {0x00, 0xa4, 0x04, 0x00, 0x0e, '2', 'P', 'A', 'Y', '.',
                        'S',  'Y',  'S',  '.',  'D',  'D', 'F', '0', '1', 0};
    if (!apdu(transfer, ppse, response, error)) return false;
    std::vector<Tlv> directory;
    if (!parse(response, directory)) {
        error = "Malformed EMV directory";
        return false;
    }
    std::vector<App> apps;
    applications(directory, apps);
    if (apps.empty()) {
        error = "No payment application found";
        return false;
    }
    std::stable_sort(apps.begin(), apps.end(), [](const App &a, const App &b) {
        return a.priority < b.priority;
    });
    for (const auto &app : apps) {
        Card candidate;
        candidate.aid = app.aid;
        candidate.label = app.label;
        Bytes select = {0x00, 0xa4, 0x04, 0x00, static_cast<uint8_t>(app.aid.size())};
        select.insert(select.end(), app.aid.begin(), app.aid.end());
        select.push_back(0);
        if (!apdu(transfer, select, response, error)) {
            if (ioFailed) return false;
            continue;
        }
        std::vector<Tlv> fci;
        if (!parse(response, fci)) {
            error = "Malformed application data";
            continue;
        }
        fields(fci, candidate);
        const auto *pdol = find(fci, 0x9f38);
        Bytes gpo;
        if (!gpoCommand(pdol ? pdol->value : Bytes{}, terminal, gpo)) {
            error = "Invalid or oversized PDOL";
            continue;
        }
        if (!apdu(transfer, gpo, response, error)) {
            if (ioFailed) return false;
            continue;
        }
        std::vector<Tlv> options;
        if (!parse(response, options)) {
            error = "Malformed processing options";
            continue;
        }
        fields(options, candidate);
        Bytes afl;
        if (const auto *format1 = find(options, 0x80)) {
            if (format1->value.size() < 2) {
                error = "Short processing options";
                continue;
            }
            afl.assign(format1->value.begin() + 2, format1->value.end());
        } else if (const auto *format2 = find(options, 0x94)) afl = format2->value;
        bool valid = afl.size() % 4 == 0;
        unsigned records = 0;
        for (size_t i = 0; valid && i < afl.size(); i += 4) {
            unsigned sfi = afl[i] >> 3, first = afl[i + 1], last = afl[i + 2];
            valid = !(afl[i] & 7) && sfi >= 1 && sfi <= 30 && first >= 1 && last >= first &&
                    afl[i + 3] <= last - first + 1;
            records += last >= first ? last - first + 1 : 0;
            valid = valid && records <= 128;
        }
        if (!valid) {
            error = "Invalid record list (AFL)";
            continue;
        }
        bool recordFailed = false;
        for (size_t i = 0; i < afl.size(); i += 4) {
            for (unsigned record = afl[i + 1]; record <= afl[i + 2]; ++record) {
                if (!apdu(transfer, {0x00, 0xb2, uint8_t(record), uint8_t(afl[i] | 4), 0}, response, error)) {
                    if (ioFailed) return false;
                    recordFailed = true;
                    continue;
                }
                std::vector<Tlv> content;
                if (!parse(response, content)) {
                    error = "Malformed card record";
                    recordFailed = true;
                    continue;
                }
                fields(content, candidate);
            }
        }
        if (!candidate.pan.empty()) {
            card = std::move(candidate);
            error.clear();
            return true;
        }
        if (!recordFailed) error = "Card did not expose a PAN";
    }
    return false;
}
} // namespace emv
