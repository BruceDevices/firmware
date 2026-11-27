#include "nrf_capture.h"

#include "modules/rf/rf_compat.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "nrf_common.h"

#include <RF24.h>
#include <esp_heap_caps.h>
#include <globals.h>

// ============================================================================
// State
// ============================================================================

static bool nrfCapReady = false;
static NrfCaptureConfig nrfCfg;
static uint8_t nrfCurrChannel = 76;
static uint8_t nrfCurrRate = NRF_RATE_1MBPS;
static uint8_t nrfCurrPower = NRF_PWR_MAX;
static uint16_t nrfStartFreqMhz = 2400;
static uint16_t nrfEndFreqMhz = 2525;
static bool nrfPromiscuous = false;
static bool nrfNormalMode = true;

// The six degenerate "noise" addresses the Spectrum view uses. With address
// width 2 and CRC off they make the address correlator accept bursts addressed
// to somebody else, which is the only way an unknown 2.4 GHz remote can be read.
static const uint8_t NRF_NOISE_ADDR[6][2] = {
    {0x55, 0x55},
    {0xAA, 0xAA},
    {0xA0, 0xAA},
    {0xAB, 0xAA},
    {0xAC, 0xAA},
    {0xAD, 0xAA}
};

// Known 2.4 GHz RGB / LED-controller profiles (see nrf_capture.h). The nRF24
// address bytes are the protocol syncword, precomputed for openReadingPipe().
const NrfSignalProfile NRF_SIGNAL_PROFILES[NRF_PROFILE_COUNT] = {
    {"RGBW (MiLight)",    {9, 40, 71}, {0x4A, 0x1A, 0x8D, 0xE2, 0x55}, 7, "FUT096 / old RGBW"},
    {"CCT (MiLight)",     {4, 39, 74}, {0xAA, 0x5A, 0x05, 0x0A, 0x55}, 7, "FUT007 / CCT"},
    {"RGB+CCT (MiLight)", {8, 39, 70}, {0x8A, 0x01, 0xE9, 0xC4, 0x56}, 9, "FUT092/089, "
                                                                     "new RGB+CCT"},
    {"RGB (MiLight)",     {3, 38, 73}, {0xD5, 0x33, 0x9B, 0x55, 0xAD}, 6, "FUT098 / RGB"},
    {"FUT020",            {6, 41, 76}, {0x55, 0xA5, 0xAA, 0x50, 0x50}, 6, "FUT020"},
};

static int nrfCurrProfile = NRF_PROFILE_GENERIC;
// Expected receive payload length: 0 = dynamic (noise mode), >0 = fixed
// (profile mode). Kept next to the profile so nrfReadOne() does not have to
// guess from the radio state.
static uint8_t nrfRxPayloadLen = 0;

// The capture ring lives in PSRAM, not in .bss. As a static array it cost 5 KB
// of INTERNAL DRAM for the whole boot, and internal DRAM is exactly what the
// Wi-Fi/BLE bring-up (and the MITM page) has to fit into. It is scratch data
// that only exists between a capture and the next Clear, so it is allocated on
// first use through nrfAllocPacketBuffer(), which prefers PSRAM.
static NrfCapturedPacket *nrfBuf = nullptr;
static int nrfBufCount = 0;
static int nrfBufDropped = 0;

/// Make sure the capture ring exists; false when neither PSRAM nor internal RAM
/// could give 5 KB (nothing can be captured then, but nothing crashes).
static bool nrfEnsureBuf() {
    if (nrfBuf != nullptr) return true;
    nrfBuf = nrfAllocPacketBuffer(NRF_CAP_MAX_PACKETS);
    return nrfBuf != nullptr;
}

static rf24_datarate_e nrfRateEnum(uint8_t rate) {
    switch (rate) {
        case NRF_RATE_2MBPS: return RF24_2MBPS;
        case NRF_RATE_250KBPS: return RF24_250KBPS;
        default: return RF24_1MBPS;
    }
}

static rf24_pa_dbm_e nrfPowerEnum(uint8_t power) {
    switch (power) {
        case NRF_PWR_MIN: return RF24_PA_MIN;
        case NRF_PWR_LOW: return RF24_PA_LOW;
        case NRF_PWR_HIGH: return RF24_PA_HIGH;
        default: return RF24_PA_MAX;
    }
}

static uint8_t nrfChannelForFreq(uint16_t mhz) {
    if (mhz < 2400) mhz = 2400;
    if (mhz > 2525) mhz = 2525;
    return (uint8_t)(mhz - 2400);
}

// ============================================================================
// Lifecycle
// ============================================================================

bool nrfCaptureReady() { return nrfCapReady; }

bool nrfCaptureInit(const NrfCaptureConfig &cfg) {
    nrfCfg = cfg;
    if (!nrf_start(NRF_MODE_SPI)) {
        nrfCapReady = false;
        return false;
    }
    nrfCapReady = NRFradio.isChipConnected();
    if (!nrfCapReady) return false;

    nrfCurrChannel = cfg.startChannel;
    nrfCurrRate = cfg.dataRate;
    nrfCurrPower = cfg.power;

    NRFradio.setDataRate(nrfRateEnum(cfg.dataRate));
    NRFradio.setPALevel(nrfPowerEnum(cfg.power));
    NRFradio.setChannel(cfg.startChannel);

    nrfPromiscuous = false;
    nrfNormalMode = true;
    return true;
}

void nrfCaptureRestoreNormal() {
    if (!nrfCapReady) return;
    NRFradio.stopListening();
    NRFradio.setAddressWidth(5);
    NRFradio.setAutoAck(true);
    NRFradio.setCRCLength(RF24_CRC_16);
    NRFradio.disableDynamicPayloads();
    NRFradio.setPayloadSize(NRF_CAP_MAX_PAYLOAD);
    nrfPromiscuous = false;
    nrfNormalMode = true;
}

uint8_t nrfCaptureGetChannel() { return nrfCurrChannel; }
uint8_t nrfCaptureGetDataRate() { return nrfCurrRate; }
uint8_t nrfCaptureGetRate() { return nrfCurrRate; }

void nrfCaptureSetChannel(uint8_t channel) {
    if (channel > 125) channel = 125;
    nrfCurrChannel = channel;
    if (nrfCapReady) NRFradio.setChannel(channel);
}

void nrfCaptureSetDataRate(uint8_t rate) {
    if (rate > NRF_RATE_250KBPS) rate = NRF_RATE_250KBPS;
    nrfCurrRate = rate;
    if (nrfCapReady) NRFradio.setDataRate(nrfRateEnum(rate));
}

void nrfCaptureSetPower(uint8_t level) {
    if (level > NRF_PWR_MAX) level = NRF_PWR_MAX;
    nrfCurrPower = level;
    if (nrfCapReady) NRFradio.setPALevel(nrfPowerEnum(level));
}

void nrfCaptureSetAddress(const uint8_t *addr, uint8_t len) {
    if (!nrfCapReady || addr == nullptr) return;
    if (len < 3 || len > 5) len = 5;
    NRFradio.stopListening();
    NRFradio.openReadingPipe(1, addr);
    NRFradio.openWritingPipe(addr);
    NRFradio.startListening();
}

void nrfCaptureSetFrequencyRange(uint16_t startMhz, uint16_t endMhz) {
    if (startMhz < 2400) startMhz = 2400;
    if (endMhz > 2525) endMhz = 2525;
    if (endMhz < startMhz) {
        uint16_t t = startMhz;
        startMhz = endMhz;
        endMhz = t;
    }
    nrfStartFreqMhz = startMhz;
    nrfEndFreqMhz = endMhz;
    nrfCfg.startChannel = nrfChannelForFreq(startMhz);
    nrfCfg.endChannel = nrfChannelForFreq(endMhz);
}

uint16_t nrfCaptureStartFreq() { return nrfStartFreqMhz; }
uint16_t nrfCaptureEndFreq() { return nrfEndFreqMhz; }

bool nrfCaptureIsPromiscuous() { return nrfPromiscuous; }
int nrfCaptureGetProfile() { return nrfCurrProfile; }

/// The receive configuration that actually sees an unknown remote: CRC off,
/// auto-ack off, address width 2 and all six pipes loaded with noise addresses.
/// This mirrors the RF24 scanner/Spectrum path that shows the remote's spikes.
static void nrfApplyNoiseConfig() {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    NRFradio.setDataRate(nrfRateEnum(nrfCurrRate));
    NRFradio.enableDynamicPayloads();
    for (uint8_t i = 0; i < 6; i++) NRFradio.openReadingPipe(i, NRF_NOISE_ADDR[i]);
    NRFradio.setChannel(nrfCurrChannel);
    NRFradio.flush_rx();
    NRFradio.startListening();
    nrfRxPayloadLen = 0;
    nrfCurrProfile = NRF_PROFILE_GENERIC;
    nrfPromiscuous = true;
    nrfNormalMode = false;
}

bool nrfCaptureSetPromiscuous(bool enable) {
    if (!nrfCapReady) return false;
    if (enable) nrfApplyNoiseConfig();
    else nrfCaptureRestoreNormal();
    return true;
}

bool nrfCaptureApplyProfile(int profileIndex, uint8_t rate) {
    if (!nrfCapReady) return false;
    if (profileIndex < 0 || profileIndex >= NRF_PROFILE_COUNT) {
        nrfApplyNoiseConfig();
        return true;
    }
    const NrfSignalProfile &p = NRF_SIGNAL_PROFILES[profileIndex];
    if (rate > NRF_RATE_250KBPS) rate = NRF_RATE_1MBPS;
    nrfCurrRate = rate;

    // A known remote uses a real address, so CRC stays off (the protocol has its
    // own trailer/checksum) but the address filter is set to the syncword.
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setDataRate(nrfRateEnum(rate));
    NRFradio.setAddressWidth(5);
    NRFradio.disableDynamicPayloads();
    NRFradio.setPayloadSize(p.payloadLen);
    NRFradio.openReadingPipe(1, p.address);
    NRFradio.setChannel(p.channels[0]);
    NRFradio.flush_rx();
    NRFradio.startListening();

    nrfRxPayloadLen = p.payloadLen;
    nrfCurrProfile = profileIndex;
    nrfPromiscuous = true;
    nrfNormalMode = false;
    return true;
}

// ============================================================================
// Capture
// ============================================================================

void nrfCaptureClear() {
    nrfBufCount = 0;
    nrfBufDropped = 0;
}

int nrfCaptureCount() { return nrfBufCount; }
int nrfCaptureDropped() { return nrfBufDropped; }

int nrfCaptureLoad(const NrfCapturedPacket *pkts, int count) {
    nrfCaptureClear();
    if (pkts == nullptr || !nrfEnsureBuf()) return 0;
    for (int i = 0; i < count && nrfBufCount < NRF_CAP_MAX_PACKETS; i++) nrfBuf[nrfBufCount++] = pkts[i];
    return nrfBufCount;
}

bool nrfCapturePacket(int index, NrfCapturedPacket *out) {
    if (index < 0 || index >= nrfBufCount || out == nullptr || nrfBuf == nullptr) return false;
    *out = nrfBuf[index];
    return true;
}

NrfCapturedPacket *nrfAllocPacketBuffer(int count) {
    if (count <= 0 || count > NRF_CAP_MAX_PACKETS) count = NRF_CAP_MAX_PACKETS;
    size_t bytes = sizeof(NrfCapturedPacket) * (size_t)count;
    NrfCapturedPacket *p = (NrfCapturedPacket *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (p == nullptr) p = (NrfCapturedPacket *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    return p;
}

static bool nrfLooksLikeNoise(const uint8_t *data, uint8_t len) {
    // The pseudo-promiscuous receiver hands over a lot of energy from other
    // 2.4 GHz sources (WiFi) as if it were a packet. A run of one repeated byte
    // is never a real remote frame; drop it so the small buffer is not filled
    // with noise before the remote's packet arrives.
    if (len < 4) return false;
    for (int i = 1; i < len; i++)
        if (data[i] != data[0]) return false;
    return true;
}

static void nrfStorePacket(const uint8_t *data, uint8_t len, uint8_t channel) {
    if (nrfLooksLikeNoise(data, len)) {
        nrfBufDropped++;
        return;
    }
    if (nrfBufCount >= NRF_CAP_MAX_PACKETS || !nrfEnsureBuf()) {
        nrfBufDropped++;
        return;
    }
    NrfCapturedPacket &p = nrfBuf[nrfBufCount++];
    p.timestamp = millis();
    p.channel = channel;
    p.freqMhz = (uint16_t)(2400 + channel);
    p.len = len > NRF_CAP_MAX_PAYLOAD ? NRF_CAP_MAX_PAYLOAD : (len == 0 ? 1 : len);
    memset(p.data, 0, sizeof(p.data));
    memcpy(p.data, data, p.len);
}

static uint8_t nrfReadOne(uint8_t *buf) {
    if (!NRFradio.available()) return 0;
    // Profile mode knows the payload length; noise mode has dynamic payloads on.
    uint8_t len = nrfRxPayloadLen;
    if (len == 0) {
        len = NRFradio.getDynamicPayloadSize();
        if (len == 0 || len > NRF_CAP_MAX_PAYLOAD) len = NRF_CAP_MAX_PAYLOAD;
    } else if (len > NRF_CAP_MAX_PAYLOAD) {
        len = NRF_CAP_MAX_PAYLOAD;
    }
    memset(buf, 0, NRF_CAP_MAX_PAYLOAD);
    NRFradio.read(buf, len);
    return len;
}

// Task 1, phase 3: patchable slot nrf_capture_raw. The capture window is the
// one decision every raw capture makes; the consumer below calls through the
// cell, so a payload changes the window live.
__attribute__((noinline, used)) uint32_t nrf_capture_raw_body(uint32_t durationMs) {
    if (durationMs == 0) return 1000;
    if (durationMs > 300000) return 300000;
    return durationMs;
}

__attribute__((noinline)) uint32_t nrf_capture_raw(uint32_t durationMs) {
    // Called directly: the patchable slot that used to wrap this does not exist
    // outside Vandeta's developer overlay.
    return nrf_capture_raw_body(durationMs);
}

// Common loop for both fixed-channel and hopping captures.
static int nrfCaptureLoop(
    uint32_t durationMs, const uint8_t *channels, int channelCount, uint32_t hopIntervalMs, bool hop,
    const NrfCaptureProgressFn &progress
) {
    VndPatchGate vndGate; // no code flip while a capture is live
    if (!nrfCapReady) return 0;
    durationMs = nrf_capture_raw(durationMs);
    // 1 ms minimum: a 3-channel remote is captured by visiting each channel
    // every few ms, so the old 5 ms floor (15 ms per round trip) could miss the
    // whole burst. The loop still samples at 1 ms granularity.
    if (hopIntervalMs < 1) hopIntervalMs = 1;

    // Capture needs a receive configuration, but it must not clobber a profile
    // the caller just applied (nrfApplyNoiseConfig would reset it to the noise
    // addresses). Only fall back to the noise configuration when the radio is in
    // its normal pipe mode.
    if (nrfNormalMode) nrfCaptureSetPromiscuous(true);
    NRFradio.flush_rx();

    // The sweep must not move the user's configured channel: a capture that
    // hops needs its own cursor, and the radio is put back on nrfCurrChannel
    // before returning. Otherwise `scan`/`capture` in sequence listens on the
    // wrong frequency (observed as a capture of 0 packets right after a scan).
    uint8_t entryChannel = nrfCurrChannel;
    uint8_t hopChannel = nrfCurrChannel;

    uint32_t start = millis();
    uint32_t lastHop = start;
    uint32_t lastProgress = start;
    int hopIndex = 0;
    uint8_t buf[NRF_CAP_MAX_PAYLOAD];

    if (channelCount > 0) NRFradio.setChannel(channels[0]);
    else NRFradio.setChannel(nrfCurrChannel);

    while ((int32_t)(start + durationMs - millis()) > 0) {
        uint8_t len = nrfReadOne(buf);
        if (len > 0) {
            uint8_t ch = channels && channelCount > 0 ? channels[hopIndex % channelCount] : hopChannel;
            nrfStorePacket(buf, len, ch);
        }

        if (hop && (millis() - lastHop) >= hopIntervalMs) {
            lastHop = millis();
            if (channels && channelCount > 0) {
                hopIndex = (hopIndex + 1) % channelCount;
                NRFradio.setChannel(channels[hopIndex]);
            } else {
                hopChannel++;
                if (hopChannel > nrfCfg.endChannel || hopChannel < nrfCfg.startChannel)
                    hopChannel = nrfCfg.startChannel;
                NRFradio.setChannel(hopChannel);
            }
        }

        // Report to the on-device screen (if any) at a rate that costs the
        // sampling loop almost nothing. A false return is an explicit abort.
        if (progress && (millis() - lastProgress) >= 100) {
            lastProgress = millis();
            NrfCaptureProgress p;
            p.elapsedMs = millis() - start;
            p.totalMs = durationMs;
            p.units = nrfBufCount;
            p.scanning = false;
            if (!progress(p)) break;
        }
        vTaskDelay(1);
    }

    nrfCaptureSetChannel(entryChannel);
    return nrfBufCount;
}

int nrfCaptureRun(uint32_t durationMs, bool forceHop, uint32_t hopIntervalMs, const NrfCaptureProgressFn &progress) {
    if (!nrfCapReady) return 0;
    nrfCaptureClear();

    // Task 1, phase 2: the capture layout, `nrf.capture_variant`.
    //   single (0) - the configured mode, exactly as before: hop only if
    //                nrfCfg.hop, listen on one channel otherwise.
    //   hop    (1) - force a sweep of the configured channel range.
    //   locked (2) - rotate only through the three channels of the applied
    //                profile. A 3-channel remote sends its whole command in a
    //                few ms, so a 0-125 sweep never spends enough time on any
    //                one of them to see it. With no profile applied there is
    //                nothing to lock to, so it stays on the current channel.
    // An explicit forceHop from the caller still wins: `nrfcap capture <ms> hop`
    // is a direct instruction, not a default.
    int variant = (int)tuneGetByName("nrf.capture_variant", nullptr);
    if (variant < 0 || variant > 2) variant = 0;
    bool hop = forceHop || nrfCfg.hop;
    if (variant == 1) hop = true;
    if (variant == 2) {
        if (nrfCurrProfile >= 0) {
            const NrfSignalProfile &p = NRF_SIGNAL_PROFILES[nrfCurrProfile];
            return nrfCaptureLoop(durationMs, p.channels, 3, hopIntervalMs, true, progress);
        }
        hop = false;
    }
    return nrfCaptureLoop(durationMs, nullptr, 0, hopIntervalMs, hop, progress);
}

int nrfCaptureRunChannels(
    const uint8_t *channels, int channelCount, uint32_t durationMs, uint32_t dwellMs,
    const NrfCaptureProgressFn &progress
) {
    if (!nrfCapReady || channels == nullptr || channelCount <= 0) return 0;
    nrfCaptureClear();
    // Rotate only through the caller's channels: a 3-channel remote sends its
    // whole command in a few ms, so a 0-125 sweep never spends enough time on
    // any one of them to see it.
    return nrfCaptureLoop(durationMs, channels, channelCount, dwellMs, true, progress);
}

int nrfCaptureChannelSummary(uint8_t *channelsOut, uint8_t *countsOut, int maxOut) {
    int n = 0;
    if (nrfBuf == nullptr) return 0;
    for (int i = 0; i < nrfBufCount && n < maxOut; i++) {
        int found = -1;
        for (int j = 0; j < n; j++) {
            if (channelsOut[j] == nrfBuf[i].channel) {
                found = j;
                break;
            }
        }
        if (found >= 0) {
            if (countsOut[found] < 255) countsOut[found]++;
        } else {
            channelsOut[n] = nrfBuf[i].channel;
            countsOut[n] = 1;
            n++;
        }
    }
    // Sort by count, descending (simple insertion sort, n is tiny).
    for (int i = 1; i < n; i++) {
        uint8_t c = channelsOut[i], k = countsOut[i];
        int j = i - 1;
        while (j >= 0 && countsOut[j] < k) {
            channelsOut[j + 1] = channelsOut[j];
            countsOut[j + 1] = countsOut[j];
            j--;
        }
        channelsOut[j + 1] = c;
        countsOut[j + 1] = k;
    }
    return n;
}

int nrfFilterMultiFreqChannels(uint8_t *channelsOut, int maxOut) {
    if (channelsOut == nullptr || maxOut <= 0) return 0;
    uint8_t chans[NRF_CAP_MAX_PACKETS];
    uint8_t counts[NRF_CAP_MAX_PACKETS];
    int n = nrfCaptureChannelSummary(chans, counts, maxOut);
    int written = 0;
    // Only channels that actually carried more than one packet count as
    // "carrying a signal" rather than noise.
    for (int i = 0; i < n && written < maxOut; i++) {
        if (counts[i] >= 2) channelsOut[written++] = chans[i];
    }
    return written;
}

// ============================================================================
// Replay
// ============================================================================

bool nrfReplayRaw(const uint8_t *data, uint8_t len, uint8_t channel, uint8_t repeat, uint8_t rate) {
    if (!nrfCapReady || data == nullptr || len == 0) return false;
    if (len > NRF_CAP_MAX_PAYLOAD) len = NRF_CAP_MAX_PAYLOAD;
    if (channel > 125) channel = 125;
    if (repeat == 0) repeat = 1;
    if (repeat > 64) repeat = 64;

    // Transmit with the same framing the capture used. With a profile applied we
    // send that profile's address and payload length; otherwise we send the raw
    // 32-byte burst with a noise address, exactly as before.
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setDataRate(nrfRateEnum(rate));
    NRFradio.setPALevel(nrfPowerEnum(nrfCurrPower));
    NRFradio.setChannel(channel);

    uint8_t txLen = NRF_CAP_MAX_PAYLOAD;
    if (nrfCurrProfile >= 0) {
        const NrfSignalProfile &p = NRF_SIGNAL_PROFILES[nrfCurrProfile];
        NRFradio.setAddressWidth(5);
        NRFradio.setPayloadSize(p.payloadLen);
        NRFradio.openWritingPipe(p.address);
        txLen = p.payloadLen;
    } else {
        NRFradio.setAddressWidth(2);
        NRFradio.setPayloadSize(NRF_CAP_MAX_PAYLOAD);
        NRFradio.openWritingPipe(NRF_NOISE_ADDR[0]);
    }
    NRFradio.flush_tx();

    uint8_t buf[NRF_CAP_MAX_PAYLOAD];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, data, len > txLen ? txLen : len);

    bool any = false;
    for (uint8_t i = 0; i < repeat; i++) {
        if (NRFradio.writeFast(buf, txLen)) any = true;
        vTaskDelay(1);
    }
    NRFradio.txStandBy();
    return any;
}

bool nrfReplayMultiFreq(
    const uint8_t *data, uint8_t len, const uint8_t *channels, int channelCount, uint8_t repeat
) {
    if (channels == nullptr || channelCount <= 0) return false;
    bool ok = false;
    for (int i = 0; i < channelCount; i++) {
        if (nrfReplayRaw(data, len, channels[i], repeat, nrfCurrRate)) ok = true;
    }
    return ok;
}

// ============================================================================
// Channel scanning
// ============================================================================

int nrfScanChannelActivity(
    uint8_t startCh, uint8_t endCh, uint32_t dwellMs, NrfChannelActivity *out, int maxOut,
    const NrfCaptureProgressFn &progress
) {
    if (!nrfCapReady || out == nullptr || maxOut <= 0) return 0;
    if (endCh > 125) endCh = 125;
    if (startCh > endCh) {
        uint8_t t = startCh;
        startCh = endCh;
        endCh = t;
    }
    if (dwellMs < 1) dwellMs = 1;

    // Receive mode with CRC off + minimum address filtering: both the carrier
    // (testCarrier) and the received-power detector (testRPD) are then a
    // reasonable proxy for "energy on this channel".
    nrfCaptureSetPromiscuous(true);

    // A scan visits every channel; put the radio back where the caller had it
    // so a capture started right after the scan listens on the right frequency.
    uint8_t entryChannel = nrfCurrChannel;

    int written = 0;
    const int samples = 200;
    int channelTotal = (int)endCh - (int)startCh + 1;
    uint32_t start = millis();
    for (uint8_t ch = startCh; ch <= endCh && written < maxOut; ch++) {
        NRFradio.setChannel(ch);
        NRFradio.flush_rx();
        int hits = 0;
        for (int i = 0; i < samples; i++) {
            if (NRFradio.testRPD() || NRFradio.testCarrier()) hits++;
            delayMicroseconds(dwellMs * 10);
        }
        out[written].channel = ch;
        out[written].freqMhz = (uint16_t)(2400 + ch);
        out[written].level = (uint8_t)((hits * 100) / samples);
        written++;

        if (progress) {
            NrfCaptureProgress p;
            p.elapsedMs = millis() - start;
            p.totalMs = 0; // duration unknown up front: the scan is counted in channels
            p.units = written;
            p.scanning = true;
            if (!progress(p)) break;
        }
    }
    nrfCaptureSetChannel(entryChannel);
    return written;
}

int nrfFindStrongestChannels(
    uint8_t *out, int maxOut, uint32_t dwellMs, const NrfCaptureProgressFn &progress
) {
    if (out == nullptr || maxOut <= 0) return 0;
    if (dwellMs < 1) dwellMs = 1;
    static NrfChannelActivity act[126];
    int n = nrfScanChannelActivity(0, 125, dwellMs, act, 126, progress);
    int written = 0;
    for (int k = 0; k < maxOut; k++) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (act[i].level == 0) continue;
            bool used = false;
            for (int j = 0; j < written; j++)
                if (out[j] == act[i].channel) used = true;
            if (used) continue;
            if (best < 0 || act[i].level > act[best].level) best = i;
        }
        if (best < 0) break;
        out[written++] = act[best].channel;
        act[best].level = 0;
    }
    return written;
}

int nrfFindBusyChannels(
    uint8_t startCh, uint8_t endCh, uint32_t dwellMs, uint8_t thresholdPct, uint8_t *out, int maxOut
) {
    NrfChannelActivity act[126];
    int n = nrfScanChannelActivity(startCh, endCh, dwellMs, act, 126);
    int written = 0;
    // Report most active first.
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < n && written < maxOut; i++) {
            if (act[i].level < thresholdPct) continue;
            bool dup = false;
            for (int j = 0; j < written; j++)
                if (out[j] == act[i].channel) dup = true;
            if (dup) continue;
            if (pass == 0) {
                // first pass writes in level order
                int best = i;
                for (int j = i; j < n; j++)
                    if (act[j].level > act[best].level && act[j].level >= thresholdPct) best = j;
                if (act[best].level < thresholdPct) continue;
                out[written++] = act[best].channel;
                act[best].level = 0;
            }
        }
        if (written > 0) break;
    }
    return written;
}

// ============================================================================
// Files
// ============================================================================

const char *nrfCaptureFolder() { return "/BruceNRF24"; }

static FS *nrfFs() {
    FS *fs = setupSdCard() ? (FS *)&SD : (FS *)&LittleFS;
    return fs;
}

static String nrfEnsureFolder(FS *fs) {
    String folder = nrfCaptureFolder();
    if (fs && !fs->exists(folder)) fs->mkdir(folder);
    return folder;
}

static String nrfNormalizePath(const String &path, const char *defExt = ".nrf") {
    String p = path;
    p.trim();
    if (p.length() == 0) return p;
    if (!p.startsWith("/")) p = String(nrfCaptureFolder()) + "/" + p;
    if (p.indexOf('.') == -1) p += defExt;
    return p;
}

bool nrfSaveSignal(const String &path, const NrfCapturedPacket *pkts, int count, const char *label) {
    FS *fs = nrfFs();
    if (fs == nullptr) return false;
    nrfEnsureFolder(fs);
    String full = nrfNormalizePath(path);
    if (full.length() == 0) return false;

    File f = fs->open(full, FILE_WRITE);
    if (!f) return false;
    f.println("# Bruce NRF24 signal v1");
    f.println(String("label=") + (label ? label : ""));
    f.println(String("count=") + String(count));
    for (int i = 0; i < count; i++) {
        String hex;
        for (int b = 0; b < pkts[i].len; b++) {
            char h[3];
            snprintf(h, sizeof(h), "%02X", pkts[i].data[b]);
            hex += h;
        }
        f.println(
            String("pkt ch=") + String(pkts[i].channel) + " freq=" + String(pkts[i].freqMhz) +
            " ts=" + String(pkts[i].timestamp) + " len=" + String(pkts[i].len) + " data=" + hex
        );
    }
    f.close();
    return true;
}

int nrfLoadSignal(const String &path, NrfCapturedPacket *out, int maxOut, String *labelOut) {
    FS *fs = nrfFs();
    if (fs == nullptr || out == nullptr || maxOut <= 0) return 0;
    String full = nrfNormalizePath(path);
    if (!fs->exists(full)) return 0;

    File f = fs->open(full, FILE_READ);
    if (!f) return 0;

    int n = 0;
    while (f.available() && n < maxOut) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        if (line.startsWith("label=") && labelOut) {
            *labelOut = line.substring(6);
            continue;
        }
        if (!line.startsWith("pkt ")) continue;

        NrfCapturedPacket p;
        memset(&p, 0, sizeof(p));
        int ch = 0, len = 0;
        unsigned long ts = 0, freq = 0;
        char hex[2 * NRF_CAP_MAX_PAYLOAD + 4] = {0};
        // pkt ch=N freq=N ts=N len=N data=HEX
        sscanf(
            line.c_str(), "pkt ch=%d freq=%lu ts=%lu len=%d data=%128s", &ch, &freq, &ts, &len, hex
        );
        if (len <= 0 || len > NRF_CAP_MAX_PAYLOAD) continue;
        p.channel = (uint8_t)ch;
        p.freqMhz = (uint16_t)freq;
        p.timestamp = (uint32_t)ts;
        p.len = (uint8_t)len;
        for (int i = 0; i < len; i++) {
            char b[3] = {hex[i * 2], hex[i * 2 + 1], 0};
            if (hex[i * 2] == 0 || hex[i * 2 + 1] == 0) break;
            p.data[i] = (uint8_t)strtol(b, nullptr, 16);
        }
        out[n++] = p;
    }
    f.close();
    return n;
}

// ============================================================================
// Analysis
// ============================================================================

NrfSignalAnalysis nrfAnalyzeSignal(const uint8_t *data, uint8_t len) {
    NrfSignalAnalysis a;
    if (data == nullptr || len == 0) return a;
    if (len > NRF_CAP_MAX_PAYLOAD) len = NRF_CAP_MAX_PAYLOAD;
    a.len = len;
    a.minByte = 0xFF;
    a.maxByte = 0x00;

    bool seen[256] = {false};
    int transitions = 0;
    int prevBit = -1;
    for (int i = 0; i < len; i++) {
        if (data[i] < a.minByte) a.minByte = data[i];
        if (data[i] > a.maxByte) a.maxByte = data[i];
        seen[data[i]] = true;
        for (int b = 7; b >= 0; b--) {
            int bit = (data[i] >> b) & 1;
            if (prevBit >= 0 && bit != prevBit) transitions++;
            prevBit = bit;
        }
    }
    for (int i = 0; i < 256; i++)
        if (seen[i]) a.uniqueBytes++;
    a.bitTransitions = transitions;

    // Repeat detection: is the payload a whole number of copies of a shorter code?
    for (int codeLen = 1; codeLen <= len / 2; codeLen++) {
        if (len % codeLen != 0) continue;
        bool match = true;
        for (int i = codeLen; i < len && match; i++) {
            if (data[i] != data[i % codeLen]) match = false;
        }
        if (match) {
            a.repeats = true;
            a.repeatLength = codeLen;
            break;
        }
    }

    if (a.repeats && a.repeatLength <= 4) a.kind = "EV1527";
    else if (a.repeats && a.repeatLength <= 8) a.kind = "PT2262";
    else if (a.repeats) a.kind = "repeated";
    else a.kind = "raw";
    return a;
}

int nrfChooseReplayChannels(const NrfCapturedPacket *pkts, int count, uint8_t *channelsOut, int maxOut) {
    if (pkts == nullptr || channelsOut == nullptr || maxOut <= 0) return 0;
    uint8_t chans[NRF_CAP_MAX_PACKETS];
    uint8_t counts[NRF_CAP_MAX_PACKETS];
    int n = 0;
    for (int i = 0; i < count; i++) {
        int found = -1;
        for (int j = 0; j < n; j++)
            if (chans[j] == pkts[i].channel) found = j;
        if (found >= 0) {
            if (counts[found] < 255) counts[found]++;
        } else {
            chans[n] = pkts[i].channel;
            counts[n] = 1;
            n++;
        }
    }
    for (int i = 1; i < n; i++) {
        uint8_t c = chans[i], k = counts[i];
        int j = i - 1;
        while (j >= 0 && counts[j] < k) {
            chans[j + 1] = chans[j];
            counts[j + 1] = counts[j];
            j--;
        }
        chans[j + 1] = c;
        counts[j + 1] = k;
    }
    int written = 0;
    for (int i = 0; i < n && written < maxOut; i++) {
        if (i < 2 || counts[i] == counts[0]) channelsOut[written++] = chans[i];
    }
    return written;
}

// ============================================================================
// Unknown-remote hunt
// ============================================================================

// How many channels a hunt listens on at once. An unknown remote transmits on a
// handful of fixed frequencies; the strongest ones found in the spectrum are the
// best guess, and 8 keeps a full round of the channel list at ~8 ms so a burst
// lasting a few ms is still caught.
#define NRF_HUNT_CHANNELS 8

/// Noise addresses of an arbitrary width. Width 2 is the exact table the
/// Spectrum uses (the documented pseudo-promiscuous configuration); wider widths
/// reuse the same degenerate 0x55/0xAA patterns, which is what makes the address
/// correlator give up on a frame addressed to somebody else.
static void nrfHuntNoiseAddr(uint8_t width, uint8_t pipe, uint8_t *out) {
    out[0] = NRF_NOISE_ADDR[pipe][0];
    for (uint8_t i = 1; i < width; i++) out[i] = (i & 1) ? 0xAA : 0x55;
}

/// Like nrfApplyNoiseConfig(), but with a caller-chosen address width and a fixed
/// receive payload length. A fixed length is what makes the hunt measurable: a
/// dynamic payload of an unknown frame is reported with the wrong size, and the
/// grouping that decides a hit needs one comparable length.
static void nrfHuntApplyConfig(uint8_t width, uint8_t payloadLen) {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(width);
    NRFradio.setDataRate(nrfRateEnum(nrfCurrRate));
    NRFradio.disableDynamicPayloads();
    NRFradio.setPayloadSize(payloadLen);
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t addr[5] = {0};
        nrfHuntNoiseAddr(width, i, addr);
        NRFradio.openReadingPipe(i, addr);
    }
    NRFradio.setChannel(nrfCurrChannel);
    NRFradio.flush_rx();
    NRFradio.startListening();
    nrfRxPayloadLen = payloadLen;
    nrfCurrProfile = NRF_PROFILE_GENERIC;
    nrfPromiscuous = true;
    nrfNormalMode = false;
}

/// Most-repeated payload currently in the capture ring. Returns the repeat count
/// and writes the index of one copy into `bestIdx` (-1 when the ring is empty).
static int nrfHuntBestRepeat(int *bestIdx) {
    int best = 0;
    if (bestIdx) *bestIdx = -1;
    for (int i = 0; i < nrfBufCount; i++) {
        int count = 0;
        const NrfCapturedPacket &a = nrfBuf[i];
        for (int j = i; j < nrfBufCount; j++) {
            const NrfCapturedPacket &b = nrfBuf[j];
            if (a.len != b.len) continue;
            bool same = true;
            for (int k = 0; k < a.len; k++) {
                if (a.data[k] != b.data[k]) {
                    same = false;
                    break;
                }
            }
            if (same) count++;
        }
        if (count > best) {
            best = count;
            if (bestIdx) *bestIdx = i;
        }
    }
    return best;
}

NrfHuntResult nrfCaptureHunt(uint32_t msPerCombo, const NrfHuntProgressFn &progress) {
    NrfHuntResult res;
    if (!nrfCapReady) return res;
    if (msPerCombo < 200) msPerCombo = 200;
    if (msPerCombo > 5000) msPerCombo = 5000;

    // Everything the hunt changes is put back on the way out, so a hunt started
    // from the RGB page or from a console session leaves the radio usable.
    uint8_t entryChannel = nrfCurrChannel;
    uint8_t entryRate = nrfCurrRate;
    int entryProfile = nrfCurrProfile;
    bool entryNormal = nrfNormalMode;

    static const uint8_t rates[3] = {NRF_RATE_1MBPS, NRF_RATE_2MBPS, NRF_RATE_250KBPS};
    static const uint8_t widths[4] = {2, 3, 4, 5};
    static const uint8_t lens[4] = {8, 16, 24, 32};
    const int steps = 3 * 4 * 4;
    int step = 0;
    bool abort = false;

    uint8_t chans[NRF_HUNT_CHANNELS] = {0};
    int nc = 0;

    for (int r = 0; r < 3 && !abort; r++) {
        // Re-scan the band once per data rate: a remote is pressed repeatedly
        // while the hunt runs, and its channels are only visible in the scan
        // that happens to overlap a press.
        nrfCurrRate = rates[r];
        nc = nrfFindStrongestChannels(chans, NRF_HUNT_CHANNELS, 8);
        if (nc <= 0) {
            // Nothing seen yet: still try, spread over the band, rather than
            // giving up before the user has pressed the remote once.
            nc = NRF_HUNT_CHANNELS;
            for (int i = 0; i < nc; i++) chans[i] = (uint8_t)(i * 126 / nc);
        }

        for (int w = 0; w < 4 && !abort; w++) {
            for (int l = 0; l < 4 && !abort; l++) {
                NrfHuntProgress hp;
                hp.step = step;
                hp.steps = steps;
                hp.rate = rates[r];
                hp.addressWidth = widths[w];
                hp.payloadLen = lens[l];
                hp.packets = 0;
                if (progress && !progress(hp)) {
                    abort = true;
                    break;
                }

                nrfHuntApplyConfig(widths[w], lens[l]);
                nrfCaptureRunChannels(chans, nc, msPerCombo, 1, [&](const NrfCaptureProgress &cp) {
                    if (!progress) return true;
                    NrfHuntProgress p = hp;
                    p.packets = (uint16_t)cp.units;
                    if (!progress(p)) {
                        abort = true;
                        return false;
                    }
                    return true;
                });
                step++;
                if (abort) break;

                int bestIdx = -1;
                int repeats = nrfHuntBestRepeat(&bestIdx);
                if (repeats >= 3 && bestIdx >= 0) {
                    res.found = true;
                    res.rate = rates[r];
                    res.addressWidth = widths[w];
                    res.payloadLen = lens[l];
                    res.repeatCount = (uint16_t)repeats;
                    res.packetCount = (uint16_t)nrfBufCount;
                    res.channel = nrfBuf[bestIdx].channel;
                    memcpy(res.payload, nrfBuf[bestIdx].data, res.payloadLen > NRF_CAP_MAX_PAYLOAD
                                                                  ? NRF_CAP_MAX_PAYLOAD
                                                                  : res.payloadLen);
                    abort = true; // stop the sweep the same way an abort does
                    break;
                }
            }
        }
    }

    res.aborted = abort && !res.found;

    // Restore the radio exactly as the caller had it.
    nrfCurrRate = entryRate;
    if (entryNormal) {
        nrfCaptureRestoreNormal();
        NRFradio.setDataRate(nrfRateEnum(entryRate));
    } else if (entryProfile >= 0) {
        nrfCaptureApplyProfile(entryProfile, entryRate);
    } else {
        nrfCaptureSetPromiscuous(true);
        NRFradio.setDataRate(nrfRateEnum(entryRate));
    }
    nrfCaptureSetChannel(entryChannel);
    return res;
}

// ============================================================================
// Shared menu helpers
// ============================================================================

/// Drop key presses that arrived while a process was running. Without this a
/// press that dismisses a result screen also fires on the menu that is rebuilt
/// behind it (SEL dismissing "Packets: 12" would immediately re-run whatever
/// entry is highlighted next).
static void nrfClearPendingKeys() {
    EscPress = false;
    SelPress = false;
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    AnyKeyPress = false;
}

/// Stock-style "dismiss the result screen" wait, hardened against stale presses.
static void nrfWaitAnyKey() {
    nrfClearPendingKeys();
    while (!check(AnyKeyPress)) vTaskDelay(pdMS_TO_TICKS(20));
    nrfClearPendingKeys();
}

/// Live screen for a running capture / channel scan. The counter is repainted a
/// few times a second, so the process looks alive instead of frozen, and the
/// "Press BACK to stop" hint is literally true: only Back aborts, every other
/// press is ignored on purpose.
struct NrfRunScreen {
    String title;
    uint32_t lastPaint = 0;
    bool aborted = false;

    void begin(const String &titleText, const String &subLine) {
        title = titleText;
        drawMainBorderWithTitle(title);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        if (subLine.length()) tft.drawString(subLine.substring(0, 34), 10, 44);
        tft.drawString("Press BACK to stop", 10, tftHeight - 20);
        nrfClearPendingKeys();
        lastPaint = millis();
    }

    /// Progress sink for nrfCaptureRun() / nrfScanChannelActivity().
    bool update(const NrfCaptureProgress &p) {
        if (check(EscPress)) {
            aborted = true;
            return false;
        }
        uint32_t now = millis();
        if (now - lastPaint < 150) return true;
        lastPaint = now;
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.fillRect(8, 66, tftWidth - 16, 32, bruceConfig.bgColor);
        if (p.scanning) {
            tft.drawString(String("Scanned: ") + String(p.units) + " ch", 10, 66);
            tft.drawString("2400-2525MHz", 10, 82);
        } else {
            tft.drawString(String("Packets: ") + String(p.units), 10, 66);
            uint32_t left = p.totalMs > p.elapsedMs ? (p.totalMs - p.elapsedMs + 999) / 1000 : 0;
            tft.drawString(String("Left: ") + String(left) + "s", 10, 82);
        }
        return true;
    }

    /// Result screen for when the process finished (or was stopped).
    void finish(const String *lines, int count) {
        drawMainBorderWithTitle(title);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        int y = 44;
        if (aborted) {
            tft.drawString("Stopped", 10, y);
            y += 18;
        }
        for (int i = 0; i < count && y < tftHeight - 26; i++) {
            tft.drawString(lines[i].substring(0, 34), 10, y);
            y += 15;
        }
        tft.drawString("Press any key", 10, tftHeight - 20);
        nrfWaitAnyKey();
    }
};

static void nrfShowLines(const String &title, const String *lines, int count) {
    drawMainBorderWithTitle(title);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int y = 42;
    for (int i = 0; i < count && y < tftHeight - 26; i++) {
        tft.drawString(lines[i].substring(0, 40), 8, y);
        y += 15;
    }
    tft.drawString("Press any key", 8, tftHeight - 20);
    nrfWaitAnyKey();
}

static void nrfDrawFrequencyAnalysis(const String &title) {
    uint8_t chans[16];
    uint8_t counts[16];
    int n = nrfCaptureChannelSummary(chans, counts, 16);
    drawMainBorderWithTitle(title);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    if (n == 0) {
        tft.drawString("No packets captured", 8, 48);
    } else {
        int maxCount = 1;
        for (int i = 0; i < n; i++)
            if (counts[i] > maxCount) maxCount = counts[i];
        int barX = 96;
        int barW = tft.width() - barX - 12;
        if (barW < 20) barW = 20;
        int y = 40;
        for (int i = 0; i < n && i < 6; i++) {
            tft.drawString(String(2400 + chans[i]) + "MHz x" + String(counts[i]), 8, y);
            int w = (int)((int32_t)counts[i] * barW / maxCount);
            if (w < 1) w = 1;
            tft.fillRect(barX, y + 2, w, 8, bruceConfig.priColor);
            y += 15;
        }
    }
    tft.drawString("Press any key", 8, tftHeight - 20);
    nrfWaitAnyKey();
}

// ============================================================================
// Raw capture menu
// ============================================================================

static void nrfEnsureInit() {
    if (nrfCaptureReady()) return;
    NrfCaptureConfig cfg;
    nrfCaptureInit(cfg);
}

static const char *nrfRateName(uint8_t r) {
    switch (r) {
        case NRF_RATE_2MBPS: return "2M";
        case NRF_RATE_250KBPS: return "250k";
        default: return "1M";
    }
}

static void nrfMenuSaveCapture() {
    if (nrfCaptureCount() == 0) {
        displayError("Nothing captured", true);
        return;
    }
    String name = keyboard("capture", 20, "Signal name:");
    if (name.length() == 0 || name == "\x1B") return;
    // Heap, not stack: this is 5 KB and the menu task cannot spare it.
    NrfCapturedPacket *pkts = nrfAllocPacketBuffer();
    if (pkts == nullptr) {
        displayError("No memory for capture", true);
        return;
    }
    int got = 0;
    for (int i = 0; i < nrfCaptureCount() && got < NRF_CAP_MAX_PACKETS; i++)
        if (nrfCapturePacket(i, &pkts[got])) got++;
    bool ok = nrfSaveSignal(name, pkts, got, "nrf24 menu");
    free(pkts);
    if (ok) displayInfo("Saved " + name, true);
    else displayError("Save failed", true);
}

static void nrfMenuReplayCapture() {
    if (nrfCaptureCount() == 0) {
        displayError("Nothing captured", true);
        return;
    }
    uint8_t chans[4];
    NrfCapturedPacket *all = nrfAllocPacketBuffer();
    if (all == nullptr) {
        displayError("No memory for replay", true);
        return;
    }
    int got = 0;
    for (int i = 0; i < nrfCaptureCount() && got < NRF_CAP_MAX_PACKETS; i++)
        if (nrfCapturePacket(i, &all[got])) got++;
    int nc = nrfChooseReplayChannels(all, got, chans, 4);
    if (nc == 0) {
        free(all);
        displayError("No channel to replay on", true);
        return;
    }

    NrfRunScreen screen;
    screen.begin("Replay", String(nc) + " freq(s) x5 repeats");
    bool ok = false;
    int sent = 0;
    for (int c = 0; c < nc && !screen.aborted; c++) {
        for (int i = 0; i < got; i++) {
            if (all[i].channel != chans[c]) continue;
            if (check(EscPress)) {
                screen.aborted = true;
                break;
            }
            if (nrfReplayRaw(all[i].data, all[i].len, chans[c], 5, nrfCaptureGetDataRate())) ok = true;
            sent++;
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.fillRect(8, 66, tftWidth - 16, 16, bruceConfig.bgColor);
            tft.drawString(String("Sent: ") + String(sent), 10, 66);
            break;
        }
    }

    free(all);

    String lines[2];
    lines[0] = String("Sent ") + String(sent) + " on " + String(nc) + " freq(s)";
    lines[1] = ok ? String("Rate: ") + nrfRateName(nrfCaptureGetDataRate()) : String("Replay failed");
    screen.finish(lines, 2);
}

static void nrfCycleProfile() {
    if (!nrfCaptureReady()) return;
    int next = nrfCaptureGetProfile() + 1;
    if (next >= NRF_PROFILE_COUNT) next = NRF_PROFILE_GENERIC;
    if (next >= 0) nrfCaptureApplyProfile(next, nrfCaptureGetRate());
    else nrfCaptureSetPromiscuous(true);
}

/// Capture an unknown 3-channel 2.4 GHz remote. A remote of this kind always
/// sends the same command on three fixed channels (the three spikes in the
/// Spectrum). With a preset selected we already know them; otherwise we find the
/// three strongest channels first. Either way the capture then rotates over just
/// those channels, so a command that lasts a few ms is actually seen.
static void nrfMenuRgbCapture() {
    if (!nrfCaptureReady()) {
        displayError("NRF24 not found", true);
        return;
    }
    int prof = nrfCaptureGetProfile();
    uint8_t rate = nrfCaptureGetRate();

    uint8_t chans[3];
    int nc = 0;

    if (prof >= 0) {
        const NrfSignalProfile &p = NRF_SIGNAL_PROFILES[prof];
        for (int i = 0; i < 3; i++) chans[nc++] = p.channels[i];
    } else {
        NrfRunScreen scan;
        scan.begin("RGB Capture", "1/2 Press the remote now");
        uint8_t found[3];
        int nf = nrfFindStrongestChannels(found, 3, 12, [&scan](const NrfCaptureProgress &p) {
            return scan.update(p);
        });
        for (int i = 0; i < nf; i++) chans[nc++] = found[i];
        if (nc == 0) {
            chans[0] = nrfCaptureGetChannel();
            nc = 1;
        }
        // The scan switched to the noise configuration; put a profile back.
        if (prof >= 0) nrfCaptureApplyProfile(prof, rate);
    }

    NrfRunScreen cap;
    String sub = "2/2";
    for (int i = 0; i < nc; i++) sub += String(" ") + String(2400 + chans[i]);
    cap.begin("RGB Capture", sub);
    nrfCaptureRunChannels(chans, nc, 6000, 2, [&cap](const NrfCaptureProgress &p) {
        return cap.update(p);
    });

    String lines[5];
    int w = 0;
    lines[w++] =
        String("Packets: ") + String(nrfCaptureCount()) + " / dropped " + String(nrfCaptureDropped());
    for (int i = 0; i < nc && w < 4; i++) {
        int c = 0;
        for (int k = 0; k < nrfCaptureCount(); k++) {
            NrfCapturedPacket p;
            if (nrfCapturePacket(k, &p) && p.channel == chans[i]) c++;
        }
        lines[w++] = String(2400 + chans[i]) + "MHz x" + String(c);
    }
    if (nrfCaptureCount() == 0) lines[w++] = "Nothing - try a preset";
    cap.finish(lines, w);
}

/// Hunt a remote whose protocol is none of the known profiles: sweep data rate,
/// address width and payload length on the busiest channels until the same frame
/// repeats. This is the answer to "the Spectrum sees my remote but capture never
/// hears it" - the chip filters by address in hardware, so a single-rate /
/// single-width capture simply never sees a remote using another setting.
static void nrfMenuHunt() {
    if (!nrfCaptureReady()) {
        displayError("NRF24 not found", true);
        return;
    }
    NrfRunScreen screen;
    screen.begin("Hunt remote", "Keep pressing the remote");

    uint32_t lastPaint = 0;
    NrfHuntResult r = nrfCaptureHunt(1200, [&](const NrfHuntProgress &p) {
        if (check(EscPress)) return false;
        uint32_t now = millis();
        if (now - lastPaint < 150) return true;
        lastPaint = now;
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.fillRect(8, 66, tftWidth - 16, 32, bruceConfig.bgColor);
        tft.drawString(
            String("Try ") + String(p.step + 1) + "/" + String(p.steps) + "  " + nrfRateName(p.rate), 10, 66
        );
        tft.drawString(
            String("addr ") + String(p.addressWidth) + "  len " + String(p.payloadLen) + "  pkts " +
                String(p.packets),
            10, 82
        );
        return true;
    });

    String lines[5];
    int w = 0;
    if (r.found) {
        lines[w++] = String("FOUND ") + nrfRateName(r.rate) + " addr " + String(r.addressWidth) + " len " +
                     String(r.payloadLen);
        lines[w++] = String("ch ") + String(2400 + r.channel) + "MHz x" + String(r.repeatCount) + " of " +
                     String(r.packetCount);
        static const char *HEXD = "0123456789ABCDEF";
        for (int start = 0; start < r.payloadLen && start < 32; start += 16) {
            String hex;
            for (int i = start; i < start + 16 && i < r.payloadLen; i++) {
                char h[3] = {HEXD[r.payload[i] >> 4], HEXD[r.payload[i] & 0x0F], 0};
                hex += h;
            }
            lines[w++] = hex;
        }
        lines[w++] = "Replay it from Replay capture";
    } else if (r.aborted) {
        lines[w++] = "Stopped before a match";
        lines[w++] = "Press the remote, then retry";
    } else {
        lines[w++] = "No repeating frame found";
        lines[w++] = "Press the remote during the hunt";
        lines[w++] = "and retry (48 combos tried)";
    }
    screen.finish(lines, w);
}

void nrfRawCaptureMenu() {
    nrfEnsureInit();
    int last = 0; // keep the cursor on the row that was just used
AGAIN:
    options = {
        {"Capture (press remote)", []() { nrfMenuRgbCapture(); }                                    },
        {"Hunt unknown remote", []() { nrfMenuHunt(); }                                             },
        {String("Preset: ") + (nrfCaptureGetProfile() >= 0
                                   ? String(NRF_SIGNAL_PROFILES[nrfCaptureGetProfile()].name)
                                   : String("Auto (find channels)")),
         []() { nrfCycleProfile(); }                                                                },
        {"Replay capture", []() { nrfMenuReplayCapture(); }                                         },
        {"Save capture", []() { nrfMenuSaveCapture(); }                                             },
    };
    addOptionToMainMenu();
    int selected = loopOptions(options, MENU_TYPE_SUBMENU, "RGB Remote", last);
    // A negative index is the physical Back button: leave this page instead of
    // rebuilding it, which used to look like "Back reloads the current menu".
    if (selected < 0) return;
    // "Main Menu" (pushed by addOptionToMainMenu) bubbles all the way out.
    if (returnToMenu) return;
    // Any other entry: rebuild, so the preset label stays accurate, and keep the
    // cursor where it was instead of jumping back to the first row.
    last = selected;
    goto AGAIN;
}
