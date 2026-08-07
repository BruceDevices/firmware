#ifndef __DUAL_DETECT_H__
#define __DUAL_DETECT_H__

// RF + IR dual-band detector.
//
// Listens to the CC1101/RMT RF receiver and the IR GPIO receiver AT THE SAME
// TIME (two FreeRTOS tasks), tells you which band fired and lets you replay,
// save or discard the capture. RF captures go to /BruceRF/*.sub, IR captures
// to /BruceIR/*.ir.
void dualDetect();

#endif
