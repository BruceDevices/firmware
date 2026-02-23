#ifndef __RF_SEND_H__
#define __RF_SEND_H__

#include "structs.h"

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

#endif
