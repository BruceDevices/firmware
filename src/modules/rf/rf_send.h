#ifndef __RF_SEND_H__
#define __RF_SEND_H__

#include "structs.h"
#include <functional>
#include <vector>

#define COUNTER_STEP 1
#define REPEAT 2

void sendCustomRF();
bool txSubFile(RfCodes &selected_code, bool hideDefaultUI = false);
bool readSubFile(FS *fs, String filepath, RfCodes &data);

void sendRfCommand(struct RfCodes rfcode, bool hideDefaultUI = false);
void RCSwitch_send(uint64_t data, unsigned int bits, int pulse = 0, int protocol = 1, int repeat = 10);

void RCSwitch_RAW_Bit_send(RfCodes data);
void RCSwitch_RAW_send(int *ptrtransmittimings);

void display_info(RfCodes &data);
void loopEmulate(RfCodes &data);

// --- Rolling Code RF ---
// Generic rolling-code TX: drives the radio at freq_mhz with the given signed
// timing vector. bw_preset is BW_OOK_270 / BW_OOK_650 (see rolling_code_db.h).
void rolling_code_tx(const std::vector<int> &timings, float freq_mhz, uint8_t bw_preset, int repeat = 3);

// Save a rolling-code signal to its .sub file (replaces keeloq_save for rolling
// paths; writes Protocol/Serial/Button/Counter/Seed/CRC/SomfyKey/Key fields).
void rolling_code_save(RfCodes &data, const String &path, FS *fs);

// Returns true if the .sub at `path` looks like a rolling code signal (carries
// a Counter: line OR a Protocol: matching the compiled-in descriptor table).
bool is_rolling_code_file(FS *fs, const String &path);

// New Rolling Code RF screens.
void createSignal();
void sendRollingCode();
void counterManager(std::function<void()> backFn = nullptr);
void rollingKeyHelp(std::function<void()> backFn = nullptr);
void loopEmulateRolling(RfCodes &data, const String &path, FS *fs);

#endif
