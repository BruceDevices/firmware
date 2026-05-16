/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_BLE_DTM_
#define H_BLE_DTM_

#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ble_dtm.h
 *
 * @brief DTM (Direct Test Mode)
 *
 * This header file provides the interface and data structures for working with
 * the Direct Test Mode (DTM) functionality in a BLE (Bluetooth Low Energy) host.
 * DTM allows for testing and validation of the BLE radio performance by enabling
 * custom transmission and reception of data packets.
 *
 * @defgroup bt_host_dtm Bluetooth Host Direct Test Mode
 * @ingroup bt_host
 * @{
 */

/**
 * @struct ble_dtm_rx_params
 * @brief Parameters for DTM RX test.
 *
 * This structure represents the parameters for a Direct Test Mode (DTM) receiver test.
 */
struct ble_dtm_rx_params {
    /** The channel to use for the RX test. */
    uint8_t channel;

    /** The PHY to use for the RX test. */
    uint8_t phy;

    /** The modulation index to use for the RX test. */
    uint8_t modulation_index;
};

/**
 * @brief Start a Direct Test Mode (DTM) receiver test.
 *
 * This function starts a DTM RX test with the provided parameters.
 *
 * @param params                The parameters for the DTM RX test.
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_dtm_rx_start(const struct ble_dtm_rx_params *params);

/**
 * @struct ble_dtm_tx_params
 * @brief Parameters for DTM TX test.
 *
 * This structure represents the parameters for a Direct Test Mode (DTM) transmitter test.
 */
struct ble_dtm_tx_params {
    /** The channel to use for the TX test. */
    uint8_t channel;

    /** The length of the data for the TX test. */
    uint8_t test_data_len;

    /** The payload to use for the TX test. */
    uint8_t payload;

    /** The PHY to use for the TX test. */
    uint8_t phy;
};

/**
 * @brief Start a Direct Test Mode (DTM) transmitter test.
 *
 * This function starts a DTM TX test with the provided parameters.
 *
 * @param params                The parameters for the DTM TX test.
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_dtm_tx_start(const struct ble_dtm_tx_params *params);

/**
 * @brief Stops a Direct Test Mode (DTM) test and retrieves the number of transmitted packets.
 *
 * This function sends a command to the Bluetooth controller to stop the currently running DTM test.
 * It retrieves the number of packets transmitted during the test and stores it in the provided `num_packets` variable.
 *
 * @param num_packets           Pointer to a `uint16_t` variable to store the number of transmitted packets.
 *                              If an error occurs, the value will be set to 0.
 *
 * @return                      0 on success;
 *                              A non-zero value on failure.
 */
int ble_dtm_stop(uint16_t *num_packets);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
