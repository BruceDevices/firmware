#ifndef __CLIENT_SCANNER_H__
#define __CLIENT_SCANNER_H__

#if !defined(LITE_VERSION)

/**
 * @brief Passive WiFi client scanner (Fing-style AP client census)
 *
 * Scans nearby networks, then listens promiscuously on each AP-bearing
 * channel and counts distinct client MACs per AP. Results are sorted by
 * client count (busiest first) so the user can pick a worthwhile target
 * for Capture Handshake. Fully passive: no deauth frames are sent.
 */
void clientScannerMenu();

#endif // LITE_VERSION

#endif
