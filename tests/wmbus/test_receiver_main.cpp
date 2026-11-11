// SPDX-License-Identifier: AGPL-3.0-or-later
// Appended to an exact copy of the production Receiver class by run.py.
static std::vector<uint8_t> testPacket(size_t length, bool tmode) {
    std::vector<uint8_t> data(length + 1);
    for (size_t i = 0; i < data.size(); ++i) data[i] = uint8_t(i * 7);
    data[0] = uint8_t(length);
    std::vector<uint8_t> raw = {0x54, 0xCD};
    for (size_t pos = 0; pos < data.size();) {
        const size_t count = pos == 0 ? 10 : std::min(size_t(16), data.size() - pos);
        raw.insert(raw.end(), data.begin() + pos, data.begin() + pos + count);
        const uint16_t c = wmbus::crc(data.data() + pos, count);
        raw.push_back(uint8_t(c >> 8)); raw.push_back(uint8_t(c));
        pos += count;
    }
    if (!tmode) return raw;
    static const uint8_t symbols[] = {0x16,0x0D,0x0E,0x0B,0x1C,0x19,0x1A,0x13,0x2C,0x25,0x26,0x23,0x34,0x31,0x32,0x29};
    std::vector<uint8_t> encoded(((raw.size() - 2) * 12 + 7) / 8, 0);
    size_t bit = 0;
    for (size_t p = 2; p < raw.size(); ++p) {
        for (int shift : {4, 0}) {
            const uint8_t symbol = symbols[(raw[p] >> shift) & 15];
            for (int k = 5; k >= 0; --k, ++bit)
                encoded[bit / 8] |= uint8_t(((symbol >> k) & 1) << (7 - bit % 8));
        }
    }
    return encoded;
}
int main() {
    assert(MAX_DEVICES == 96);
    // Vary FIFO delivery from one byte at a time to near the FIFO capacity.
    // The maximum-size frame crosses the 256-byte hardware-counter boundary.
    for (size_t length : {size_t(9), size_t(25), size_t(126), size_t(255)}) {
      for (bool tmode : {false, true}) {
        const auto raw = testPacket(length, tmode);
        for (size_t batch = 1; batch <= 60; ++batch) {
            ELECHOUSE_cc1101.reset();
            ELECHOUSE_cc1101.regs[0x12] = 0xAB;
            Receiver receiver;
            Session stats;
            wmbus::Frame frame;
            assert(receiver.begin());
            int accepted = 0;
            size_t radioMicros = 0;
            for (size_t pos = 0; pos < raw.size();) {
                const size_t n = std::min(batch, raw.size() - pos);
                ELECHOUSE_cc1101.feed(raw.data() + pos, n);
                pos += n;
                // Poll often enough to drain a FIFO batch without assuming one
                // poll consumes it all (the first three bytes establish length).
                for (unsigned i = 0; i < 6; ++i) {
                    const int result = receiver.poll(frame, stats);
                    assert(result >= 0); accepted += result;
                }
                radioMicros += n * 80; // 100 kbit/s, not one millisecond per byte
                testTime += uint32_t(radioMicros / 1000);
                radioMicros %= 1000;
            }
            if (accepted != 1 || frame.bytes.size() != length + 1)
                std::cerr << "length=" << length << " batch=" << batch << " accepted=" << accepted << " rejected=" << stats.rejected << " timeout=" << stats.timeouts << "\n";
            assert(accepted == 1 && frame.bytes.size() == length + 1);
            assert(frame.rssi == -122 && frame.lqi == 51);
            assert(stats.rejected == 0 && stats.overflows == 0);
            assert(!ELECHOUSE_cc1101.emptiedDuringRx);
            receiver.end();
            assert(ELECHOUSE_cc1101.regs[0x12] == 0xAB);
            assert(ELECHOUSE_cc1101.state == 1);
        }
      }
    }
    ELECHOUSE_cc1101.reset();
    Receiver receiver;
    Session stats;
    wmbus::Frame frame;
    assert(receiver.begin());
    ELECHOUSE_cc1101.overflow = true;
    assert(receiver.poll(frame, stats) == 0 && stats.overflows == 1);
    const uint8_t prefix[] = {0x54,0xCD,0x30,0x44};
    ELECHOUSE_cc1101.feed(prefix, sizeof(prefix));
    assert(receiver.poll(frame, stats) == 0);
    testTime += 81;
    assert(receiver.poll(frame, stats) == 0 && stats.timeouts == 1);
    const uint8_t bad[] = {0x54,0x00,0x30,0x44};
    ELECHOUSE_cc1101.feed(bad, sizeof(bad));
    assert(receiver.poll(frame, stats) == 0 && stats.rejected == 1);
    ELECHOUSE_cc1101.stuck = true;
    ELECHOUSE_cc1101.overflow = true;
    const uint32_t start = testTime;
    assert(receiver.poll(frame, stats) == -1);
    assert(testTime - start <= 30);
    ELECHOUSE_cc1101.stuck = false;
    receiver.end();
    assert(receiver.begin());
    ELECHOUSE_cc1101.feed(prefix, sizeof(prefix));
    ELECHOUSE_cc1101.failRead = true;
    assert(receiver.poll(frame, stats) == -1); // A failed SPI read must not reuse old bytes.
    ELECHOUSE_cc1101.failRead = false;
    receiver.end();
    std::cout << "PASS: production Receiver with simulated CC1101: FIFO fragmentation, long frames, no empty FIFO during RX, register restore, overflow/timeout recovery\n";
}
