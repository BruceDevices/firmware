// Synthetic card transcripts only. Run via test_emv_protocol.py.
#include "emv_protocol.h"
#include <cassert>
#include <iostream>
#include <random>
using namespace emv;
Bytes cat(Bytes a, const Bytes &b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}
Bytes tlv(uint32_t tag, Bytes data) {
    Bytes r;
    if (tag > 0xffff) r.push_back(tag >> 16);
    if (tag > 0xff) r.push_back(tag >> 8);
    r.push_back(tag);
    if (data.size() > 255) {
        r.push_back(0x82);
        r.push_back(data.size() >> 8);
    } else if (data.size() >= 128) r.push_back(0x81);
    r.push_back(data.size());
    return cat(r, data);
}
Bytes ok(Bytes data) { return cat(data, {0x90, 0x00}); }
const Bytes Aid = {0xa0, 0, 0, 0, 0x99, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
Bytes directory(Bytes aid = Aid) { return tlv(0x6f, tlv(0xa5, tlv(0xbf0c, tlv(0x61, tlv(0x4f, aid))))); }
Bytes select(Bytes aid = Aid) { return cat(cat({0, 0xa4, 4, 0, uint8_t(aid.size())}, aid), {0}); }
const Bytes Ppse = {0,   0xa4, 4,   0,   14,  '2', 'P', 'A', 'Y', '.',
                    'S', 'Y',  'S', '.', 'D', 'D', 'F', '0', '1', 0};
struct Step {
    Bytes command, response;
};
struct Script {
    std::vector<Step> steps;
    size_t pos = 0;
    bool run(const Bytes &c, Bytes &r) {
        assert(pos < steps.size());
        if (steps[pos].command != c) {
            std::cerr << "Unexpected APDU at step " << pos << '\n';
            std::abort();
        }
        r = steps[pos++].response;
        return true;
    }
    bool read(Card &card, std::string &error) {
        bool result = readCard(
            [this](const Bytes &c, Bytes &r) {
                return run(c, r);
        },
            {{0x26, 0x10, 0x10}, {1, 2, 3, 4}},
            card,
            error
        );
        assert(pos == steps.size());
        return result;
    }
};
Bytes responseFrame(Bytes payload, bool extended = false) {
    Bytes data = cat({0xd5, 0x41}, payload), f = {0, 0, 0xff};
    if (extended) {
        uint8_t hi = data.size() >> 8, lo = data.size();
        f = cat(f, {0xff, 0xff, hi, lo, uint8_t(-hi - lo)});
    } else f = cat(f, {uint8_t(data.size()), uint8_t(-data.size())});
    f = cat(f, data);
    uint8_t sum = 0;
    for (auto b : data) sum += b;
    return cat(f, {uint8_t(-sum), 0});
}
int main() {
    Card card;
    std::string error;
    // Unknown scheme, 16-byte AID, no PDOL, format 80, PAN only in second record,
    // holder in third record, 19-digit PAN with F padding, optional dates absent.
    Script records{
        {
         {Ppse, ok(directory())},
         {select(), ok(tlv(0x6f, {}))},
         {{0x80, 0xa8, 0, 0, 2, 0x83, 0, 0}, ok(tlv(0x80, {0, 0, 0x10, 1, 3, 0}))},
         {{0, 0xb2, 1, 0x14, 0}, ok(tlv(0x70, tlv(0x9f42, {0x08, 0x40})))},
         {{0, 0xb2, 2, 0x14, 0},
             ok(tlv(0x70, tlv(0x5a, {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x9f})))},
         {{0, 0xb2, 3, 0x14, 0}, ok(tlv(0x70, tlv(0x5f20, {'T', 'E', 'S', 'T'})))},
         }
    };
    assert(records.read(card, error));
    assert(
        card.pan == "1234567890123456789" && card.holder == "TEST" && card.validTo.empty() && card.aid == Aid
    );
    std::cout << "PASS: variable AID, all AFL records, PAN padding, optional fields\n";
    // Non-Visa PDOL including unknown entries and a card-specific order; format 77,
    // odd-length track 2 PAN and expiry begin in the middle of a byte.
    Bytes pdol = {0x9f, 0x37, 4, 0xdf, 1, 3, 0x9f, 0x66, 4, 0x9a, 3};
    Bytes gpo = {0x80, 0xa8, 0, 0,    16, 0x83, 14,   1,    2,    3,    4,
                 0,    0,    0, 0x79, 0,  0x40, 0x80, 0x26, 0x10, 0x10, 0};
    Script track{
        {{Ppse, ok(directory())},
         {select(), ok(tlv(0x6f, tlv(0xa5, tlv(0x9f38, pdol))))},
         {gpo,
          ok(tlv(0x77, tlv(0x57, {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x5d, 0x29, 0x12, 0x10, 0x1f})))}}
    };
    assert(track.read(card, error) && card.pan == "123456789012345" && card.validTo == "12/29");
    std::cout << "PASS: dynamic non-Visa PDOL, unknown tags, format 77, odd track 2\n";
    // Standard 6Cxx correction then 61xx GET RESPONSE; 9000 inside a TLV is data.
    Bytes dir = directory();
    Script status{
        {{Ppse, {0x6c, 0x20}},
         {cat(Bytes(Ppse.begin(), Ppse.end() - 1), {0x20}),
          cat(Bytes(dir.begin(), dir.begin() + 4), {0x61, 0x10})},
         {{0, 0xc0, 0, 0, 0x10}, ok(Bytes(dir.begin() + 4, dir.end()))},
         {select(), ok(tlv(0x6f, tlv(0xa5, tlv(0xdf01, {0x90, 0}))))},
         {{0x80, 0xa8, 0, 0, 2, 0x83, 0, 0}, ok(tlv(0x77, tlv(0x5a, {0x12, 0x34, 0x56, 0x78})))}}
    };
    assert(status.read(card, error));
    std::cout << "PASS: APDU 6C/61 handling and embedded status bytes\n";
    // First application rejected; select second AID rather than assuming first works.
    Bytes aid2 = {0xa0, 0, 0, 0, 1};
    Bytes two = tlv(0x6f, cat(tlv(0x61, tlv(0x4f, Aid)), tlv(0x61, tlv(0x4f, aid2))));
    Script multiple{
        {{Ppse, ok(two)},
         {select(), {0x6a, 0x82}},
         {select(aid2), ok(tlv(0x6f, {}))},
         {{0x80, 0xa8, 0, 0, 2, 0x83, 0, 0}, ok(tlv(0x77, tlv(0x5a, {0x12, 0x34, 0x56, 0x78})))}}
    };
    assert(multiple.read(card, error) && card.aid == aid2);
    std::cout << "PASS: multiple applications and selection fallback\n";
    for (const Bytes &afl : {
             Bytes{8, 1, 2},
             Bytes{8, 3, 1, 0},
             Bytes{0, 1, 1, 0},
             Bytes{8, 1, 255, 0},
             Bytes{8, 1, 1, 2}
    }) {
        Script invalid{
            {{Ppse, ok(directory())},
             {select(), ok(tlv(0x6f, {}))},
             {{0x80, 0xa8, 0, 0, 2, 0x83, 0, 0}, ok(tlv(0x80, cat({0, 0}, afl)))}}
        };
        assert(!invalid.read(card, error) && card.pan.empty());
    }
    // Invalid PDOL and a card that does not provide PAN must report failure.
    for (const Bytes &pd : {
             Bytes{0x9f},
             Bytes{0x9f, 0x66},
             Bytes{0x9f, 0x66, 241}
    }) {
        Script invalid{
            {{Ppse, ok(directory())}, {select(), ok(tlv(0x6f, tlv(0xa5, tlv(0x9f38, pd))))}}
        };
        assert(!invalid.read(card, error));
    }
    Script empty{
        {{Ppse, ok(directory())},
         {select(), ok(tlv(0x6f, {}))},
         {{0x80, 0xa8, 0, 0, 2, 0x83, 0, 0}, ok(tlv(0x80, {0, 0}))}}
    };
    assert(!empty.read(card, error) && error == "Card did not expose a PAN");
    std::cout << "PASS: malformed AFL/PDOL and missing PAN rejected\n";
    // PN532 normal/extended frames include trailing padding as real bus reads do.
    for (size_t length : {size_t(4), size_t(240), size_t(259)}) {
        Bytes payload(length, 0x55), out;
        payload[1] = 0x90;
        payload[2] = 0;
        Bytes f = responseFrame(payload, length > 250);
        assert(pn532Response(f, 0x40, out) && out == payload);
        for (size_t cut = 0; cut < f.size(); ++cut)
            assert(!pn532Response(Bytes(f.begin(), f.begin() + cut), 0x40, out));
        f.resize(280, 0xff);
        assert(pn532Response(f, 0x40, out) && out == payload);
        f[10] ^= 1;
        assert(!pn532Response(f, 0x40, out));
    }
    assert(pn532Command({0x40, 1, 0, 0xa4, 4, 0, 0}).size() == 15);
    assert(pn532Command(Bytes(254, 0)).empty());
    std::cout << "PASS: complete PN532 lengths, checksums, truncation, extended frames\n";
    // Exercise bounds with arbitrary card-controlled directory bytes under sanitizers.
    std::mt19937 random(42);
    for (unsigned i = 0; i < 5000; ++i) {
        Bytes bad(random() % 600);
        for (auto &b : bad) b = random();
        Script fuzz{{{Ppse, ok(bad)}}};
        // Random data almost surely has no valid application; reject before further APDUs.
        assert(!fuzz.read(card, error));
    }
    unsigned calls = 0;
    assert(
        !readCard(
            [&](const Bytes &, Bytes &) {
                ++calls;
                return false;
            },
            {},
            card,
            error
        ) &&
        calls == 1
    );
    calls = 0;
    assert(
        !readCard(
            [&](const Bytes &, Bytes &out) {
                ++calls;
                if (calls == 1) out = ok(directory());
                else if (calls == 2) out = ok(tlv(0x6f, {}));
                else if (calls == 3) out = ok(tlv(0x80, {0, 0, 8, 1, 128, 0}));
                else return false;
                return true;
            },
            {},
            card,
            error
        ) &&
        calls == 4
    );
    std::cout << "PASS: 5000 malformed inputs under ASan/UBSan; I/O failure\n";
}
