/*
    AudioOutputPDM

    Copyright (C) 2025  Earle F. Philhower, III

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(ESP32)

#include "AudioOutputPDM.h"

#if SOC_I2S_SUPPORTS_PDM_TX

AudioOutputPDM::AudioOutputPDM(int pin) : AudioOutputI2S() {
    i2sOn = false;
    pdmPin = pin;
    channels = 2;
    hertz = 44100; // Needs to be application-set
    SetGain(1.0);
    SetBuffers(); // Default DMA sizes
}

AudioOutputPDM::~AudioOutputPDM() {
    stop();
}

bool AudioOutputPDM::SetRate(int hz) {
    if (hertz == hz) {
        return true;
    }
    hertz = hz;
    if (i2sOn) {
        auto adj = AdjustI2SRate(hz);  // Possibly scale for NoDAC
        i2s_pdm_tx_clk_config_t clk_cfg;
        clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG((uint32_t)adj);
        clk_cfg.up_sample_fp = 960;
        clk_cfg.up_sample_fs = 480;
        i2s_channel_disable(_tx_handle);
        i2s_channel_reconfig_pdm_tx_clock(_tx_handle, &clk_cfg);
        i2s_channel_enable(_tx_handle);
    }
    return true;
}

bool AudioOutputPDM::begin() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = _buffers;
    chan_cfg.dma_frame_num = _bufferWords;
    assert(ESP_OK == i2s_new_channel(&chan_cfg, &_tx_handle, nullptr));

    i2s_pdm_tx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(AdjustI2SRate(hertz)),
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .clk = I2S_GPIO_UNUSED,
            .dout = (gpio_num_t)pdmPin,
#if SOC_I2S_PDM_MAX_TX_LINES > 1
            .dout2 = I2S_GPIO_UNUSED,
#endif
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    pdm_cfg.slot_cfg.data_fmt = I2S_PDM_DATA_FMT_PCM;
    pdm_cfg.clk_cfg.up_sample_fp = 960;
    pdm_cfg.clk_cfg.up_sample_fs = 480;
    assert(ESP_OK == i2s_channel_init_pdm_tx_mode(_tx_handle, &pdm_cfg));

    // Fill w/0s to start off
    int16_t a[2] = {0, 0};
    size_t written = 0;
    do {
        i2s_channel_preload_data(_tx_handle, (void*)a, sizeof(a), &written);
    } while (written);

    i2sOn = (ESP_OK == i2s_channel_enable(_tx_handle));
    return true;
}

#endif

#endif
