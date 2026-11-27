#ifndef __MORE_ATKS_H__
#define __MORE_ATKS_H__

// Additional 802.11 attacks for the WiFi menu.
//
// These complement the classic deauth/beacon-spam engine in wifi_atks.cpp with
// more management and EAPOL frame types: probe requests, authentication frames,
// EAPOL-Start/Logoff, channel switch announcements, bad association responses
// and disassociation frames. Everything is built from scratch here and sent
// through wifiRawTx() (the retrying wrapper over esp_wifi_80211_tx), which is
// the same primitive the rest of Bruce's injection uses.
//
// All entry points are headless: they run to their own deadline and return, so
// they can be called from a menu (which paints the status/result frame) or from
// the console. The one exception is that a caller must not assume WiFi is up -
// the attack core calls wifi_atk_setWifi() itself when needed.
//
// HONEST REACH: management frames (probe/auth/assoc/CSA/disassoc) are not
// protected, so a spoofed one can be accepted by a client or an AP. The EAPOL
// frames are 802.11 *data* frames, so on a WPA2/WPA3 network the receiver's
// radio needs them encrypted with the pairwise key the board does not have -
// they are transmitted and counted, but a protected network may drop them.

#include <Arduino.h>
#include <functional>
#include <vector>

/// Progress callback: (frames sent, elapsed ms). Called every ~250 ms.
typedef std::function<void(uint32_t frames, uint32_t elapsedMs)> MoreProgressFn;

/// Result returned by every attack.
struct MoreAtkResult {
    bool ok = false;
    uint32_t frames = 0;  // frames the driver accepted
    uint32_t seconds = 0; // wall clock the attack ran
    uint8_t channel = 0;
    int targets = 0;         // APs/clients involved
    String detail = "";      // short human readable summary
    const char *error = nullptr; // set when ok is false
};

/// Runtime bounds shared by every attack.
#define MORE_ATK_MIN_MS 1000
#define MORE_ATK_MAX_MS 300000

// ---------------------------------------------------------------------------
// Flood attacks (no specific target: they scan and hit everything they find)
// ---------------------------------------------------------------------------

/// Authentication frame flood at every AP in range (open-system auth requests
/// with randomised source MACs).
MoreAtkResult more_auth_flood(uint32_t durationMs, const MoreProgressFn &progress = nullptr);

/// Probe request flood. `ssid` empty = one random SSID per burst; otherwise the
/// given SSID is broadcast in every probe request.
MoreAtkResult more_probe_flood(
    uint32_t durationMs, const String &ssid = "", bool randomSSID = true, const MoreProgressFn &progress = nullptr
);

/// EAPOL-Start flood at every AP in range (prompts the AP to send message 1,
/// also useful to provoke handshakes).
MoreAtkResult more_eapol_start_flood(uint32_t durationMs, const MoreProgressFn &progress = nullptr);

// ---------------------------------------------------------------------------
// Targeted attacks (one AP, identified by BSSID + channel)
// ---------------------------------------------------------------------------

/// Send EAPOL-Logoff frames so a client tears its association down.
MoreAtkResult more_eapol_logoff(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress = nullptr
);

/// Impersonate the AP with Channel Switch Announcement beacons to move its
/// clients onto `newChannel` (a far/DFS channel makes them drop off).
MoreAtkResult more_channel_switch(
    const uint8_t bssid[6], const char *ssid, uint8_t channel, uint8_t newChannel, uint32_t durationMs,
    const MoreProgressFn &progress = nullptr
);

/// Malformed management frames: association responses with a failure status code
/// and beacons carrying an invalid element, spoofed from the AP.
MoreAtkResult more_bad_message(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress = nullptr
);

/// Authentication frame flood at a single AP.
MoreAtkResult more_auth_attack(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress = nullptr
);

/// Disassociation frame flood from the AP to all of its clients (broadcast).
MoreAtkResult more_disassoc_attack(
    const uint8_t bssid[6], uint8_t channel, uint32_t durationMs, const MoreProgressFn &progress = nullptr
);

/// DHCP starvation at one AP.
///
/// Unlike the frame-level attacks above, this one has to join the network first:
/// an AP drops data frames from a MAC that never associated, so raw 802.11
/// injection cannot reach a real DHCP server. It associates with `ssid` /
/// `password` (empty password = open network), then sends DHCP DISCOVERs from
/// port 68 to the broadcast address, each with a new random client hardware
/// address in chaddr - the field a DHCP server keys its lease on. The board's
/// own link keeps its real MAC, so the AP forwards every DISCOVER.
///
/// Reach: works on any network the board can join. It exhausts the server's
/// address pool; it does not authenticate or take over the network.
/// With `untilBack`, the duration and the no-offer stop are ignored and the
/// attack runs until BACK; the menu entry uses that.
MoreAtkResult more_dhcp_starvation(
    const uint8_t bssid[6], const String &ssid, const String &password, uint8_t channel, uint32_t durationMs,
    const MoreProgressFn &progress = nullptr, bool untilBack = false
);

// ---------------------------------------------------------------------------
// Menu entry points
// ---------------------------------------------------------------------------

// Flood attack entry points. They are menu entries of the main WiFi > Wifi Atks
// menu (the old nested "More Attacks" page was removed), so each one paints its
// own status/result frame and runs for the standard menu duration.
void moreAuthFloodMenu();
void moreProbeFloodMenu();
void moreEapolStartMenu();

// Per-target attacks, reached directly from the Target Attacks menu (the old
// nested "More Attacks" page was removed). Each one paints its own status and
// result frame and runs for the standard menu duration, exactly like the other
// Target Attacks entries.
void moreTargetEapolLogoff(const String &tssid, const String &mac, uint8_t channel);
void moreTargetChannelSwitch(const String &tssid, const String &mac, uint8_t channel);
void moreTargetBadMessage(const String &tssid, const String &mac, uint8_t channel);
void moreTargetAuthAttack(const String &tssid, const String &mac, uint8_t channel);
void moreTargetDisassoc(const String &tssid, const String &mac, uint8_t channel);
void moreTargetDhcpStarve(const String &tssid, const String &mac, uint8_t channel);

// ---------------------------------------------------------------------------
// Helpers used by the console commands and the menu
// ---------------------------------------------------------------------------

/// Parse "AA:BB:CC:DD:EE:FF" into 6 bytes. Returns false on a bad string.
bool moreParseMac(const String &mac, uint8_t out[6]);

/// Human readable name for a targeted attack id (0..5).
const char *moreTargetAtkName(int id);

/// Run one targeted attack by id so the console and the menu share a code path.
/// `newChannel` is only used by the channel-switch attack.
MoreAtkResult moreRunTargetAtk(
    int id, const String &ssid, const String &mac, uint8_t channel, uint8_t newChannel, uint32_t durationMs,
    const MoreProgressFn &progress = nullptr, const String &password = ""
);

#endif
