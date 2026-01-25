#ifndef __FM_H__
#define __FM_H__

#include "core/display.h"
#include "core/mykeyboard.h"
#include <Wire.h>
#include <globals.h>

// Global FM state variables
extern uint16_t fm_station;
extern bool is_running;

// Missing function declarations added here:
void fm_banner();
void fm_options(uint16_t f_min, uint16_t f_max, bool reserved);

void fm_live_run(bool reserved);
void fm_ta_run();
bool fm_begin();
bool fm_tune(bool silent = false);
void fm_stop();
void fm_spectrum();
uint16_t fm_scan();
void set_frq(uint16_t frq);
void set_auto_scan(bool new_value);

#endif
