#ifndef __TELNET_SCAN_H__
#define __TELNET_SCAN_H__

// Scans the local /24 for hosts answering on TCP port 23.
//
// Different from WiFi > TelNET, which opens a session to a host you already
// know: this one finds the hosts. It needs an established connection or an AP
// with clients, because the subnet to walk is taken from the current IP.
void telnetScanMenu();

#endif
