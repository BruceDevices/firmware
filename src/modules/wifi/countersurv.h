#ifndef __COUNTERSURV_H__
#define __COUNTERSURV_H__

#include <Arduino.h>

// Task 3: counter-surveillance dashboard ("am I being attacked?").
// Task 5: probe-request IE fingerprinting that survives MAC rotation.
//
// One promiscuous capture pass feeds both: management frames for the blue-team
// checks and probe requests for the fingerprints. Everything is fixed-size
// arrays so a long run cannot fragment the heap.
namespace CounterSurv {

// `seconds` of capture, channel hopping. `fingerprint` also fills the IE hash
// table. The report is plain text, one finding per line.
String run(int seconds, bool fingerprint);

// Task 3, the BLE half: one passive/active BLE scan, grouped by payload
// signature rather than by address, so a tracker that rotates its MAC is still
// one device. Reports how many addresses one payload used and the cadence
// between rotations - that is what tells "a tag is following me" apart from
// "there is a beacon in the room".
String bleCadence(int seconds);

// Task 3, the panel: one page that runs the WiFi pass and/or the BLE cadence and
// shows the report on the encoder display.
void dashboard();

// Just the BLE half, run once and shown on the panel (WiFi > Wifi Atks).
void blePage();

} // namespace CounterSurv

#endif
