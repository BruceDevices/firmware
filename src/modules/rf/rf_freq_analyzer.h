#ifndef __RF_FREQ_ANALYZER_H__
#define __RF_FREQ_ANALYZER_H__

// Flipper-style frequency analyzer for the CC1101 modules.
//
// Sweeps the selected sub-GHz band, draws one bar per frequency step and keeps
// a peak marker on the strongest signal seen. Painted with Bruce's own colours
// on a black background. Needs a CC1101; any other RF module shows a clean
// error instead of a blank screen.
void rf_freq_analyzer();

#endif
