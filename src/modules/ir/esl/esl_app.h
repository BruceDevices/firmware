/* Bruce UI entry point for the ESL infrared transmitter. */
#pragma once

#include "esl_proto.h"

void startEslTx();

/* Prompts for a tag barcode; fills plid + profile. False if the user's entry
 * is unusable (wrong length, unknown type, or a segment tag). */
bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile);
