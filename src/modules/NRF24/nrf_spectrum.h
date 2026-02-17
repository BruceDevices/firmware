/**
 * @file nrf_spectrum.h
 * @brief Enhanced 2.4 GHz spectrum analyzer using nRF24L01+ RPD register.
 *
 * Scans 126 channels (2.400-2.525 GHz, full ISM band) with color
 * gradient visualization, peak hold markers, and adaptive layout
 * for all Bruce-supported screen sizes.
 *
 * Supports PA+LNA modules (E01-ML01SP2) for improved sensitivity.
 */
#ifndef __NRF_SPECTRUM_H
#define __NRF_SPECTRUM_H

#include "modules/NRF24/nrf_common.h"

/// Number of channels to scan (0-125 = 126 channels, full nRF24L01+ range)
#define NRF_SPECTRUM_CHANNELS 126

/// Main spectrum analyzer function (interactive, exits on ESC)
void nrf_spectrum();

/// Perform one scanning sweep and draw results.
/// @param web  If true, returns JSON string of channel values.
/// @return JSON string if web=true, empty string otherwise.
String scanChannels(bool web = false);

#endif // __NRF_SPECTRUM_H
