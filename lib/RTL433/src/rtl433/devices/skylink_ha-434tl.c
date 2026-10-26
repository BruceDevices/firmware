/* skylink off-brand wireless motion sensor and alarm system on 433.3MHz
 * Skylink HA-434TL
 *

   This motion sensor is pretty primitive, but the price is good.  It only sends
   messages when it see's motion, and has no motion clear message.  It also does
   not appear to send battery levels or low battery.  It does send regular heartbeats though ( every 2 hours )

   2018-09-26 21:25:07 :	Skylink motion sensor   :	00000
   pulse_demod_ppm(): Skylink HA-434TL motion sensor
   bitbuffer:: Number of rows: 2
   [00] { 0}           :
   [01] {17} be 3e 80  : 10111110 00111110 1
   Pulse data: 19 pulses
   [  0] Pulse:  507, Gap:  498, Period: 1005   Assuming signal preamble
   [  1] Pulse:  134, Gap:  381, Period:  515   1
   [  2] Pulse:  135, Gap:  126, Period:  261   0
   [  3] Pulse:  134, Gap:  382, Period:  516   1
   [  4] Pulse:  135, Gap:  383, Period:  518   1
   [  5] Pulse:  134, Gap:  382, Period:  516   1
   [  6] Pulse:  135, Gap:  382, Period:  517   1
   [  7] Pulse:  134, Gap:  382, Period:  516   1
   [  8] Pulse:  135, Gap:  125, Period:  260   0
   [  9] Pulse:  135, Gap:  125, Period:  260   0
   [ 10] Pulse:  134, Gap:  126, Period:  260   0
   [ 11] Pulse:  134, Gap:  382, Period:  516   1
   [ 12] Pulse:  134, Gap:  382, Period:  516   1
   [ 13] Pulse:  135, Gap:  381, Period:  516   1
   [ 14] Pulse:  136, Gap:  381, Period:  517   1
   [ 15] Pulse:  136, Gap:  381, Period:  517   1
   [ 16] Pulse:  135, Gap:  125, Period:  260   0
   [ 17] Pulse:  135, Gap:  383, Period:  518   1
   [ 18] Pulse:  134, Gap: 5071, Period: 5205   End of message

   This is transmitted roughly every 2 hours

   {"time" : "2018-09-27 06:55:50", "model" : "Skylink motion sensor", "code" : "5e3e8"}
   pulse_demod_ppm(): Skylink HA-434TL motion sensor
   bitbuffer:: Number of rows: 2
   [00] { 3} e0        : 111
   [01] {17} 5e 3e 80  : 01011110 00111110 1
   Pulse data: 22 pulses
   [  0] Pulse:   12, Gap:  269, Period:  281
   [  1] Pulse:   13, Gap:  269, Period:  282
   [  2] Pulse:   10, Gap:  235, Period:  245
   [  3] Pulse:  506, Gap:  499, Period: 1005  Assuming signal preamble
   [  4] Pulse:  132, Gap:  127, Period:  259  0
   [  5] Pulse:  132, Gap:  385, Period:  517  1
   [  6] Pulse:  132, Gap:  127, Period:  259  0
   [  7] Pulse:  133, Gap:  384, Period:  517  1
   [  8] Pulse:  132, Gap:  385, Period:  517
   [  9] Pulse:  132, Gap:  385, Period:  517
   [ 10] Pulse:  132, Gap:  384, Period:  516
   [ 11] Pulse:  132, Gap:  128, Period:  260
   [ 12] Pulse:  132, Gap:  128, Period:  260
   [ 13] Pulse:  132, Gap:  127, Period:  259
   [ 14] Pulse:  132, Gap:  384, Period:  516
   [ 15] Pulse:  133, Gap:  384, Period:  517
   [ 16] Pulse:  133, Gap:  383, Period:  516
   [ 17] Pulse:  133, Gap:  384, Period:  517
   [ 18] Pulse:  133, Gap:  383, Period:  516
   [ 19] Pulse:  133, Gap:  128, Period:  261
   [ 20] Pulse:  132, Gap:  384, Period:  516
   [ 21] Pulse:  133, Gap: 5061, Period: 5194
   new_tmplate callback:

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "decoder.h"

static int skylink_motion_callback(r_device* decoder, bitbuffer_t* bitbuffer) {
  data_t* data;
  uint8_t* b;
  int code;
  int motion;
  int raw;
  char code_str[6];
  char raw_str[6];

  for (int i = 0; i < bitbuffer->num_rows; ++i) {
    b = bitbuffer->bb[i];
    if (bitbuffer->bits_per_row[i] != 17)
      continue;

    {
      if (decoder->verbose > 1)
        fprintf(stderr, "%s: rows %d row %i bits %d\n", __func__, bitbuffer->num_rows, i, bitbuffer->bits_per_row[i]);
      // [01] {17} 5e 3e 80  : 01011110 00111110 1  -- No motion
      // [01] {17} be 3e 80  : 10111110 00111110 1  -- Motion

      // The last row should be the signal
      if (i + 1 != bitbuffer->num_rows) {
        continue;
      }
      motion = (b[0] >> 5);
      raw = code = ((b[0]) << 12) | (b[1] << 4) | (b[2] >> 4);
      code = ((b[0] & 0x1F) << 12) | (b[1] << 4) | (b[2] >> 4);

      if (code == 0)
        return 0; // Abort early for bad signal

      sprintf(code_str, "%05x", code);
      sprintf(raw_str, "%05x", raw);

      if ((motion != 5) && (motion != 2)) {
        return 0; // Abort early for bad signal
      }

      motion = (motion == 5);

      /* Get time now */
      data = data_make(
          "model", "", DATA_STRING, "Skylink_HA-434TL_motion",
          "motion", "", DATA_INT, motion ? 1 : 0,
          "id", "", DATA_STRING, code_str,
          "raw", "", DATA_STRING, raw_str,
          NULL);

      decoder_output_data(decoder, data);
      return 1;
    }
  }
  return 0;
}

static char const* output_fields[] = {
    "model",
    "id",
    "motion",
    "raw",
    NULL};

r_device skylink_motion = {
    .name = "Skylink HA-434TL motion sensor",
    .modulation = OOK_PULSE_PPM,
    .short_width = 600, // Divide by 4 from DEBUG ouput
    .long_width = 1700,
    .gap_limit = 2000,
    .reset_limit = 10000,
    .decode_fn = &skylink_motion_callback,
    .disabled = 0,
    .fields = output_fields};
