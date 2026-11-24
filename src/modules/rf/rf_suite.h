#ifndef __RF_SUITE_H__
#define __RF_SUITE_H__

// RF > RF Suite: the sub-GHz tools under one roof, drawn the way the IR Suite
// draws itself (gradient title band, gradient highlight on the selected row,
// one-line footer hint), so the two suites read as the same product.
//
// It is a launcher, not a replacement: each row opens the page that already
// exists (Frequency Analyzer, Field Meter, Packet Monitor, Wardriver), and
// BACK returns here rather than dropping the user into the main menu.
void rf_suite();

#endif
