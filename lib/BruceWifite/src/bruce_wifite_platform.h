// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
static inline BaseType_t wf_queue_put(QueueHandle_t q,const void *v,unsigned ms) { return xQueueSend(q,v,pdMS_TO_TICKS(ms)); }
static inline BaseType_t wf_queue_get(QueueHandle_t q,void *v,unsigned ms) { return xQueueReceive(q,v,pdMS_TO_TICKS(ms)); }
static inline void wf_delay(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)?pdMS_TO_TICKS(ms):1); }
#ifdef __cplusplus
extern "C" {
#endif
bool wlan_hal_survey_begin(bool exclusive);
bool wlan_hal_survey_run(void (*fn)(void*),void *ctx);
void wlan_hal_survey_end(void);
const char *wlan_hal_survey_status(void);
size_t wf_checkpoint_read(const char *path,char *text,size_t cap);
bool wf_checkpoint_write(const char *path,const char *text);
void wf_checkpoint_remove(const char *path);
bool wf_platform_prepare(void);
void wf_platform_restore(void);
#ifdef __cplusplus
}
#endif
