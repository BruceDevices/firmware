// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#ifdef CONFIG_ASYNC_TCP_LOG_CUSTOM
// The user must provide the following macros in AsyncTCPLoggingCustom.h:
//   async_tcp_log_e, async_tcp_log_w, async_tcp_log_i, async_tcp_log_d, async_tcp_log_v
#include <AsyncTCPLoggingCustom.h>

#elif defined(CONFIG_ASYNC_TCP_DEBUG)
// Local Debug logging
#include <HardwareSerial.h>
#define async_tcp_log_e(format, ...) Serial.printf("E async_tcp %s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define async_tcp_log_w(format, ...) Serial.printf("W async_tcp %s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define async_tcp_log_i(format, ...) Serial.printf("I async_tcp %s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define async_tcp_log_d(format, ...) Serial.printf("D async_tcp %s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define async_tcp_log_v(format, ...) Serial.printf("V async_tcp %s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#else
// Framework-based logging

/**
 * LibreTiny specific configurations
 */
#if defined(LIBRETINY)
#include <Arduino.h>
#define async_tcp_log_e(format, ...) log_e(format, ##__VA_ARGS__)
#define async_tcp_log_w(format, ...) log_w(format, ##__VA_ARGS__)
#define async_tcp_log_i(format, ...) log_i(format, ##__VA_ARGS__)
#define async_tcp_log_d(format, ...) log_d(format, ##__VA_ARGS__)
#define async_tcp_log_v(format, ...) log_v(format, ##__VA_ARGS__)

/**
 * Arduino specific configurations
 */
#elif defined(ARDUINO)
#if defined(USE_ESP_IDF_LOG)
#include <esp_log.h>
#define async_tcp_log_e(format, ...) ESP_LOGE("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_w(format, ...) ESP_LOGW("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_i(format, ...) ESP_LOGI("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_d(format, ...) ESP_LOGD("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_v(format, ...) ESP_LOGV("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#include <esp32-hal-log.h>
#define async_tcp_log_e(format, ...) log_e(format, ##__VA_ARGS__)
#define async_tcp_log_w(format, ...) log_w(format, ##__VA_ARGS__)
#define async_tcp_log_i(format, ...) log_i(format, ##__VA_ARGS__)
#define async_tcp_log_d(format, ...) log_d(format, ##__VA_ARGS__)
#define async_tcp_log_v(format, ...) log_v(format, ##__VA_ARGS__)
#endif  // USE_ESP_IDF_LOG

/**
 * ESP-IDF specific configurations
 */
#else
#include <esp_log.h>
#define async_tcp_log_e(format, ...) ESP_LOGE("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_w(format, ...) ESP_LOGW("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_i(format, ...) ESP_LOGI("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_d(format, ...) ESP_LOGD("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define async_tcp_log_v(format, ...) ESP_LOGV("async_tcp", "%s() %d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif  // !LIBRETINY && !ARDUINO

#endif  // CONFIG_ASYNC_TCP_LOG_CUSTOM
