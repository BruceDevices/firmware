#ifndef __NRF_CAPTURE_H
#define __NRF_CAPTURE_H

// NRF24 raw signal capture, replay and analysis.
//
// This is the engine behind both the JavaScript bindings (nrf24.captureRaw,
// nrf24.replayRaw, ...) and the NRF24 > Raw Capture menu, so the scripted and
// on-device paths share one implementation.
//
// PSEUDO-PROMISCUOUS MODE (CRC off, auto-ack off, 2-byte "noise" addresses)
//   The nRF24L01 hardware filters received packets by address, so it cannot be
//   put in a true promiscuous mode. There is, however, a well-known receive
//   configuration that makes the chip hand over packets addressed to somebody
//   else: CRC off, auto-ack off, the address width set to the shortest value the
//   chip accepts (2 bytes), and all six pipes loaded with the deliberately
//   degenerate "noise" addresses (0x55/0xAA patterns). Those addresses confuse
//   the address correlator into treating the RF preamble as the address, so a
//   burst from a remote with an unknown address is delivered anyway. This is the
//   exact configuration the Spectrum view uses to SEE a 2.4 GHz signal, and it
//   is the only configuration under which an unknown remote can be read.
//
//   The older capture code used a 3-byte fixed address (E7 E7 E7). That still
//   requires the first three address bytes to match, which is why a remote could
//   be visible in Spectrum but never be captured. Do not go back to it:
//     * 2-byte width + the six noise addresses -> any burst is received
//     * CRC disabled        -> payloads with a bad CRC are delivered too
//     * auto-ack is disabled   -> no ACK requirement
//     * a fixed 32-byte payload keeps the FIFO readable for any length
//   It is still not true promiscuous mode (the payload may include a few bits of
//   preamble/address framing), so captures are treated as raw bytes to replay.

#include <Arduino.h>
#include <functional>

/// Capture buffer bounds.
#define NRF_CAP_MAX_PACKETS 128
#define NRF_CAP_MAX_PAYLOAD 32

// Interior DRAM is what Wi-Fi and BLE need, and it is the scarcest resource on
// this board. Callers that only need a temporary packet array must NOT declare
// `NrfCapturedPacket pkts[NRF_CAP_MAX_PACKETS]` as a static/local: that is 5 KB
// of internal RAM each (the JS bindings plus the console verbs had seven of
// them, ~35 KB) and it made esp_wifi_init and the NimBLE bring-up fail with
// ESP_ERR_NO_MEM. Use nrfAllocPacketBuffer() instead - it prefers PSRAM.

/// A single received raw packet.
struct NrfCapturedPacket {
    uint32_t timestamp; // millis() when it arrived
    uint16_t freqMhz;   // 2400 + channel
    uint8_t channel;    // 0..125
    uint8_t len;        // payload length actually read (1..32)
    uint8_t data[NRF_CAP_MAX_PAYLOAD];
};

/// Data rates, indexed the same way as the JS setDataRate() argument.
enum NrfDataRate { NRF_RATE_1MBPS = 0, NRF_RATE_2MBPS = 1, NRF_RATE_250KBPS = 2 };

/// Transmit power, indexed the same way as the JS setPowerLevel() argument.
enum NrfPowerLevel { NRF_PWR_MIN = 0, NRF_PWR_LOW = 1, NRF_PWR_HIGH = 2, NRF_PWR_MAX = 3 };

/// Capture configuration.
struct NrfCaptureConfig {
    uint8_t startChannel = 0;
    uint8_t endChannel = 125;
    uint8_t dataRate = NRF_RATE_1MBPS;
    uint8_t power = NRF_PWR_MAX;
    bool promiscuous = true; // CRC off + minimum address width
    bool hop = false;        // step through the channel range while listening
};

/// Frequency analysis result for one channel.
struct NrfChannelActivity {
    uint8_t channel;
    uint16_t freqMhz;
    uint8_t level; // 0..100, mapped from RPD (carrier detected) sampling
};

// ---------------------------------------------------------------------------
// Known 2.4 GHz RGB / LED-controller profiles
// ---------------------------------------------------------------------------

/// A known on-air parameter set for the family of 2.4 GHz LED/RGB remotes that
/// use a Nordic-clone (PL1167 / nRF24-compatible) radio. These remotes always
/// transmit the SAME command on THREE fixed channels (the three spikes seen in
/// the Spectrum), so a capture that locks onto the right three channels hears
/// the whole command while a 0-125 sweep almost never does.
///
/// MiLight / LimitlessLED is the best documented member of that family and many
/// other brands (EGLO, FUT, and no-name strips) clone it. Each entry is the
/// nRF24 address derived from the protocol's syncword plus its three channels.
struct NrfSignalProfile {
    const char *name;
    uint8_t channels[3];
    uint8_t address[5]; // nRF24 pipe address, in openReadingPipe() order
    uint8_t payloadLen; // over-the-air payload length in bytes
    const char *about;
};

#define NRF_PROFILE_COUNT 5
#define NRF_PROFILE_GENERIC (-1) // no profile: raw noise-address promiscuous RX
extern const NrfSignalProfile NRF_SIGNAL_PROFILES[NRF_PROFILE_COUNT];

/// Heuristic analysis of one captured payload.
struct NrfSignalAnalysis {
    int len = 0;
    int uniqueBytes = 0;
    int bitTransitions = 0;
    uint8_t minByte = 0;
    uint8_t maxByte = 0;
    bool repeats = false;       // payload is a repeated short code
    int repeatLength = 0;       // code length when `repeats` is true
    const char *kind = "unknown"; // "RGB", "PT2262", "EV1527", "repeated", "raw"
};

// ---------------------------------------------------------------------------
// Module lifecycle
// ---------------------------------------------------------------------------

/// Bring the NRF24 up (SPI) and apply a capture configuration.
bool nrfCaptureInit(const NrfCaptureConfig &cfg);

/// True once the radio answered SPI.
bool nrfCaptureReady();

/// Put the radio back into the normal pipe mode used by send/receive.
void nrfCaptureRestoreNormal();

/// Current transceiver settings kept by this module (used by the JS setters).
uint8_t nrfCaptureGetChannel();
uint8_t nrfCaptureGetDataRate();
void nrfCaptureSetChannel(uint8_t channel);
void nrfCaptureSetDataRate(uint8_t rate);
void nrfCaptureSetPower(uint8_t level);
void nrfCaptureSetAddress(const uint8_t *addr, uint8_t len);
void nrfCaptureSetFrequencyRange(uint16_t startMhz, uint16_t endMhz);
uint16_t nrfCaptureStartFreq();
uint16_t nrfCaptureEndFreq();

/// Enable/disable the pseudo-promiscuous receive configuration.
bool nrfCaptureSetPromiscuous(bool enable);
bool nrfCaptureIsPromiscuous();

/// Current data rate remembered by the module (kept for replay/profile apply).
uint8_t nrfCaptureGetRate();

// ---------------------------------------------------------------------------
// Capture
// ---------------------------------------------------------------------------

/// Progress reported while a capture or a channel scan runs.
struct NrfCaptureProgress {
    uint32_t elapsedMs = 0;
    uint32_t totalMs = 0; // 0 while the total is unknown (channel scan)
    int units = 0;        // packets captured, or channels scanned
    bool scanning = false; // true = channel scan, false = capture
};

/// Live progress callback used by the on-device menus. Called about every
/// 100 ms while a capture or a channel scan runs; returning false aborts the
/// operation early, which is how the "Press BACK to stop" hint is honoured - a
/// Back press is the one input that interrupts a running capture, every other
/// button is ignored.
typedef std::function<bool(const NrfCaptureProgress &p)> NrfCaptureProgressFn;

/// Listen for `durationMs`, filling the internal buffer. When the configured
/// config has `hop` set (or `forceHop` is true) the radio steps through the
/// configured channel range every `hopIntervalMs`.
/// Returns the number of packets captured.
int nrfCaptureRun(
    uint32_t durationMs, bool forceHop = false, uint32_t hopIntervalMs = 20,
    const NrfCaptureProgressFn &progress = nullptr
);

/// Listen for `durationMs` while rotating ONLY through the given channel list,
/// spending `dwellMs` on each. This is how a 3-channel RGB remote is captured:
/// a 0-125 sweep with a 20 ms dwell spends 2.5 s between visits to any one
/// channel, while a command burst lasts only a few ms, so it is never seen. A
/// 3-entry list with a 2 ms dwell visits every channel every 6 ms.
/// Returns the number of packets captured.
int nrfCaptureRunChannels(
    const uint8_t *channels, int channelCount, uint32_t durationMs, uint32_t dwellMs = 2,
    const NrfCaptureProgressFn &progress = nullptr
);

/// Apply a known RGB/LED-controller profile (index into NRF_SIGNAL_PROFILES):
/// sets the pipe address, the three channels and the data rate, and turns on the
/// matching receive configuration. Pass NRF_PROFILE_GENERIC to go back to the
/// raw noise-address promiscuous configuration.
bool nrfCaptureApplyProfile(int profileIndex, uint8_t rate = NRF_RATE_1MBPS);

/// Index of the profile last applied (NRF_PROFILE_GENERIC when none).
int nrfCaptureGetProfile();

/// Sample every channel and return the `maxOut` busiest ones, strongest first.
/// Used to discover the three channels an unknown remote transmits on.
int nrfFindStrongestChannels(
    uint8_t *out, int maxOut, uint32_t dwellMs = 8, const NrfCaptureProgressFn &progress = nullptr
);

// ---------------------------------------------------------------------------
// Unknown-remote hunt
// ---------------------------------------------------------------------------

/// Result of nrfCaptureHunt(): the on-air parameters of a remote whose protocol
/// is NOT one of the known profiles. Everything a replay needs is here.
struct NrfHuntResult {
    bool found = false;
    bool aborted = false;
    uint8_t rate = NRF_RATE_1MBPS;   // NrfDataRate index of the hit
    uint8_t addressWidth = 2;        // 2..5 bytes the chip was matching on
    uint8_t payloadLen = 0;          // fixed RX payload length the frame arrived in
    uint8_t channel = 0;             // channel the frame was heard on
    uint16_t repeatCount = 0;        // how many identical copies were seen
    uint16_t packetCount = 0;        // packets captured in the combo that hit
    uint8_t payload[NRF_CAP_MAX_PAYLOAD] = {0}; // raw bytes (trailing zeros padded)
};

/// Progress of a hunt; returning false aborts it (that is how BACK stops one).
struct NrfHuntProgress {
    int step = 0;          // combo being tried, 0-based
    int steps = 0;         // total combos in this sweep
    uint8_t rate = 0;
    uint8_t addressWidth = 0;
    uint8_t payloadLen = 0;
    uint16_t packets = 0;  // packets seen in the combo so far
};

typedef std::function<bool(const NrfHuntProgress &p)> NrfHuntProgressFn;

/// Hunt an unknown 2.4 GHz remote by sweeping the receive parameters, because a
/// single data rate + address width is why the Spectrum sees a remote that raw
/// capture never hears: the chip filters by address in hardware, and a remote
/// using another rate/width is dropped before software runs.
///
/// It first finds the busiest channels (pressing the remote during the scan is
/// what makes them visible), then tries every data rate (1M/2M/250k) x address
/// width (2..5) x payload length (8/16/24/32) on just those channels, ~2 s each.
/// A combo is a hit when the same frame repeats >= 3 times, because a remote
/// repeats its command and ambient 2.4 GHz noise does not. Stops on the first
/// hit. `msPerCombo` is clamped to 200..5000 ms and defaults to 1200 ms, so a
/// full 48-combo sweep takes about a minute plus the channel scans.
///
/// The radio is put back exactly where the caller had it (rate, channel and the
/// applied profile, or the noise configuration) on every exit path, including an
/// abort. The packets of the combo that hit stay in the capture buffer so they
/// can be replayed straight away.
NrfHuntResult nrfCaptureHunt(uint32_t msPerCombo = 1200, const NrfHuntProgressFn &progress = nullptr);

int nrfCaptureCount();
int nrfCaptureDropped();
bool nrfCapturePacket(int index, NrfCapturedPacket *out);
void nrfCaptureClear();

/// A throwaway packet array for callers that need one only while they run.
/// Allocated from PSRAM first, internal RAM only as a fallback. Returns nullptr
/// if both fail. The caller normally caches the pointer in a function-local
/// static, so the cost is paid once.
NrfCapturedPacket *nrfAllocPacketBuffer(int count = NRF_CAP_MAX_PACKETS);
/// Replace the capture buffer with packets loaded from a file (for replay).
int nrfCaptureLoad(const NrfCapturedPacket *pkts, int count);

/// Group the capture by channel: returns the number of distinct channels seen
/// and writes their ids/levels. Used by the frequency-analysis display.
int nrfCaptureChannelSummary(uint8_t *channelsOut, uint8_t *countsOut, int maxOut);

/// Detect channels that carry correlated traffic (the two frequencies an RGB
/// remote transmits on). Writes pairs of channels into `pairsOut`
/// (2 * maxPairs entries) and returns the number of pairs found.
int nrfFilterMultiFreqChannels(uint8_t *channelsOut, int maxOut);

// ---------------------------------------------------------------------------
// Replay
// ---------------------------------------------------------------------------

/// Transmit one payload on one channel `repeat` times.
bool nrfReplayRaw(const uint8_t *data, uint8_t len, uint8_t channel, uint8_t repeat, uint8_t rate);

/// Transmit one payload on several channels (dual-frequency replay).
bool nrfReplayMultiFreq(const uint8_t *data, uint8_t len, const uint8_t *channels, int channelCount, uint8_t repeat);

// ---------------------------------------------------------------------------
// Channel scanning
// ---------------------------------------------------------------------------

/// Sample the carrier-detect flag on every channel in [startCh, endCh] and map
/// it to a 0..100 activity level. Returns the number of channels written.
int nrfScanChannelActivity(
    uint8_t startCh, uint8_t endCh, uint32_t dwellMs, NrfChannelActivity *out, int maxOut,
    const NrfCaptureProgressFn &progress = nullptr
);

/// Scan across the configured range and return the channels whose activity is
/// above `thresholdPct`, most active first.
int nrfFindBusyChannels(uint8_t startCh, uint8_t endCh, uint32_t dwellMs, uint8_t thresholdPct, uint8_t *out, int maxOut);

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

/// Save a capture as a text signal profile. `/BruceNRF24/` is created on demand.
bool nrfSaveSignal(const String &path, const NrfCapturedPacket *pkts, int count, const char *label);

/// Load a profile into `out`. Returns packets loaded (0 on error) and sets
/// `labelOut` when non-null.
int nrfLoadSignal(const String &path, NrfCapturedPacket *out, int maxOut, String *labelOut);

/// Default folder for signal profiles.
const char *nrfCaptureFolder();

// ---------------------------------------------------------------------------
// Analysis / RGB helpers
// ---------------------------------------------------------------------------

/// Analyse one payload (protocol guess, repeat detection, bit statistics).
NrfSignalAnalysis nrfAnalyzeSignal(const uint8_t *data, uint8_t len);

/// Pick the replay channels for a capture: the two channels with the most
/// packets when two are present, otherwise the single busiest one.
int nrfChooseReplayChannels(const NrfCapturedPacket *pkts, int count, uint8_t *channelsOut, int maxOut);

// ---------------------------------------------------------------------------
// On-device menus
// ---------------------------------------------------------------------------

/// NRF24 > Raw Capture: capture, list, save/load, replay and analyse.
void nrfRawCaptureMenu();

#endif
