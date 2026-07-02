#pragma once
#if !defined(LITE_VERSION)
#include <Arduino.h>

// Convert captured handshake PCAPs to hashcat .hc22000 format
// Reads .pcap files from /BrucePCAP/handshakes/, extracts PMKID
// and 4-way handshake data, writes .hc22000 files
//
// Usage:
//   eapol_export_menu();  // Shows menu, user selects file to convert

void eapol_export_menu();
#endif
