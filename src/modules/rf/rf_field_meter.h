#ifndef __RF_FIELD_METER_H__
#define __RF_FIELD_METER_H__

// Task 2.1 - RF field-strength meter ("find the transmitter").
//
// A coarse sweep of the standard sub-GHz channels finds the strongest source,
// then the meter locks to it and samples continuously: big live level, peak
// hold, min / average / max since reset, a bar in the theme ramp and a beep
// whose rate rises as you walk towards the transmitter. Walk the room, the
// beep speeds up, the bar grows; SEL resets the statistics, BACK leaves.
//
// RSSI on a CC1101 is coarse and moves with temperature and the antenna, so the
// screen shows a relative level next to the raw dBm - never an absolute power
// claim. Receive only: the radio is tuned and read, nothing is transmitted.
void rf_field_meter();

#endif
