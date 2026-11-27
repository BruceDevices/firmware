#ifndef __MITM_UC_H__
#define __MITM_UC_H__

#include <Arduino.h>

#if !defined(LITE_VERSION)

/**
 * @file mitm_uc.h
 * @brief ucmitm - the MITM page without a Wi-Fi password.
 *
 * The normal MITM attack needs the station link: every frame net_spoof.cpp sends
 * goes out encrypted with the AP's pairwise key, which is exactly why it works
 * on WPA2. It also means the board must know the password first.
 *
 * ucmitm does not need it. It turns the same request into an evil twin:
 *
 *   1. pick the network from the same scan list - only its SSID, BSSID and
 *      channel are kept, nothing is joined;
 *   2. listen on that channel (management/data headers are in the clear even on
 *      WPA2) to learn the MAC addresses of the clients using it, so the attack
 *      can target all of them or one;
 *   3. start an open twin AP with the same SSID, answer every DNS query with
 *      the twin's address and serve the payload (page or 302) on every request;
 *   4. deauthenticate the real AP's clients at 4 Hz, from the real AP's BSSID,
 *      so they fall back to the twin - where the payload is waiting.
 *
 * What it cannot do: a WPA2 client's *data* frames still cannot be read or
 * injected without the key, so there is no ARP pair, no transparent forwarding
 * and no HTTPS endpoint on the twin. Those are the connected engine's job
 * (mitm_menu.cpp) and stay there. Everything a captive portal can reach - the
 * client's first plain-HTTP request, its DNS lookups, its captive-portal probe -
 * is served by the twin.
 *
 * If the network *does* have a saved password on the board, mitm_menu.cpp
 * escalates to the full connected attack instead (see mitmRunCore()).
 */

/// Clients one ucmitm run remembers from the discovery scan.
#define MITM_UC_MAX_CLIENTS 32

// ---------------------------------------------------------------------------
// The toggle and the picked network
// ---------------------------------------------------------------------------

/// State of the "ucmitm" row on the MITM page.
bool mitmUcEnabled();
void mitmUcSetEnabled(bool on);

/**
 * @brief Show the AP scan and remember the picked network, without joining it.
 *
 * The picker is the same one the connected page uses; only the handler differs:
 * this one stores SSID / BSSID / channel and never asks for a password.
 */
void mitmUcPickNetwork();

/**
 * @brief Fill an empty pick from the network the station is already on.
 *
 * ucmitm needs an SSID/BSSID/channel, not a password, so the connected network
 * is the natural default: a run started without visiting the picker used to
 * fail with "pick a network for ucmitm first" while the board was associated
 * with the very network it was asked to attack.
 */
void mitmUcEnsureNetwork();

/// "MyNet ch6", or "pick one" when nothing is selected yet.
String mitmUcNetworkLabel();

/// True when a network with a usable channel has been picked.
bool mitmUcNetworkOk();

String mitmUcSsid();
uint8_t mitmUcChannel();
/// Copy the picked BSSID into @p out; false when the scan did not report one.
bool mitmUcBssid(uint8_t out[6]);

// ---------------------------------------------------------------------------
// Targets
// ---------------------------------------------------------------------------

/**
 * @brief Listen on the target channel and collect the clients seen there.
 *
 * 802.11 data frame headers carry the transmitter address in the clear, so the
 * MACs of the AP's clients are readable on WPA2 without the key. Only frames
 * whose receiver/BSSID is the picked AP are counted, which keeps neighbouring
 * networks on the same channel out of the list.
 *
 * @param ms how long to listen (1000..20000)
 * @return number of clients found, also kept for mitmUcClientCount()
 */
int mitmUcScanClients(uint32_t ms = 6000);

/**
 * @brief Passive watch with no AP picked: stations and probe requests on one
 *        channel.
 *
 * The fallback for a recon run that cannot do more: an ucmitm payload with no
 * page to serve, and a connected recon whose ARP pairing did not come up. Data
 * frame transmitters and probe-request SSIDs are in the clear, so no key and
 * no association are involved.
 *
 * @param channel 1..14, or 0 for the channel the station is on
 * @param ms listening time (1000..30000)
 * @return number of stations seen, also kept for mitmUcClientCount()
 */
int mitmUcPassiveScan(uint8_t channel, uint32_t ms = 6000);

int mitmUcClientCount();
/// "aa:bb:cc:dd:ee:ff" for one discovered client.
String mitmUcClientLabel(int idx);
/// Pick "All devices" (-1) or one client by index; false when out of range.
bool mitmUcSetTargetClient(int idx);
void mitmUcTargetAll();
/// "All devices (deauth + twin)", or the chosen client's MAC.
String mitmUcTargetLabel();
/// Run the discovery scan, then let the user pick all clients or one.
void mitmUcPickTarget();

/// SSIDs seen in probe requests during the last scan (the recon list).
int mitmUcProbeCount();
String mitmUcProbeSsid(int idx);

/// Pick a network by SSID or BSSID without the picker (console/tests).
bool mitmUcSelectNetwork(const String &name, String &error);

// ---------------------------------------------------------------------------
// The attack
// ---------------------------------------------------------------------------

/**
 * @brief Run the twin AP with the payload that is already armed.
 *
 * The page/redirect comes from the loaded payload: an injected page first, then
 * its `offline page`, then its first redirect rule - the same order the
 * connected run uses when it has no internet path, because a twin has none
 * either. A payload with none of those cannot feed a twin and is reported.
 *
 * @param durationMs 0 = until BACK (capped by the portal engine at 10 minutes)
 * @param summary receives the counters of the run (clients, requests, deauth)
 * @return "" when the run finished, otherwise an "err: ..." line
 */
String mitmUcRun(const struct MitmScriptResult &payload, uint32_t durationMs, bool draw, String &summary);

#endif // !LITE_VERSION

#endif // __MITM_UC_H__
