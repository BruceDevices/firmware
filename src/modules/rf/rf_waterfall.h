#pragma once

#include "rf_utils.h"

void rf_waterfall_boundary_freq(float &boundary);
void rf_waterfall_run();

extern float m_rf_waterfall_start_freq;
extern float m_rf_waterfall_end_freq;

void rf_waterfall();
