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

#ifndef H_BLE_HS_LOG_
#define H_BLE_HS_LOG_

/**
 * @file ble_hs_log.h
 *
 * @brief Bluetooth Host Log
 *
 * This header file defines macros and functions used for logging messages
 * within the BLE Host Stack.
 *
 * @defgroup bt_host_log Bluetooth Host Log
 * @ingroup bt_host
 * @{
 */

#include "nimble/porting/nimble/include/modlog/modlog.h"
#include "nimble/porting/nimble/include/log/log.h"

/* Only include the logcfg header if this version of newt can generate it. */
#if MYNEWT_VAL(NEWT_FEATURE_LOGCFG)
#include "nimble/porting/nimble/include/logcfg/logcfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct os_mbuf;

/**
 * @brief Macro for logging messages at a specified log level.
 *
 * The BLE_HS_LOG macro allows logging messages with different severity levels,
 * such as DEBUG, INFO, WARN, ERROR or CRITICAL.
 *
 * @param lvl           The log level of the message.
 * @param ...           The format string and additional arguments for the log message.
 */
#define BLE_HS_LOG(lvl, ...) \
    BLE_HS_LOG_ ## lvl(__VA_ARGS__)

/**
 * @brief Macro for logging a Bluetooth address at a specified log level.
 *
 * The BLE_HS_LOG_ADDR macro allows logging Bluetooth addresses in the format
 *  "XX:XX:XX:XX:XX:XX" at different severity levels, such as DEBUG, INFO, WARN, ERROR or CRITICAL.
 *
 *  @param lvl          The log level of the message.
 *  @param addr         The Bluetooth address to be logged.
 */
#define BLE_HS_LOG_ADDR(lvl, addr)                      \
    BLE_HS_LOG_ ## lvl("%02x:%02x:%02x:%02x:%02x:%02x", \
                       (addr)[5], (addr)[4], (addr)[3], \
                       (addr)[2], (addr)[1], (addr)[0])


/**
 * @brief Logs the content of an `os_mbuf` structure.
 *
 * This function iterates over each byte in the provided `os_mbuf` and logs its
 * value in hexadecimal format using the `BLE_HS_LOG` macro with the log level
 * set to DEBUG.
 *
 * @param om            The `os_mbuf` to log.
 */
void ble_hs_log_mbuf(const struct os_mbuf *om);

/**
 * @brief Logs the content of a flat buffer.
 *
 * This function iterates over each byte in the provided buffer and logs its
 * value in hexadecimal format using the `BLE_HS_LOG` macro with the log level
 * set to DEBUG.
 *
 * @param data          Pointer to the buffer to log.
 * @param len           Length of the buffer.
 */
void ble_hs_log_flat_buf(const void *data, int len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
