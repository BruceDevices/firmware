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

#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../nimble/include/nimble/nimble_port.h"

static TaskHandle_t host_task_h = NULL;

UBaseType_t nimble_port_freertos_get_hs_hwm(void) {
    if (!host_task_h) {
        return 0;
    }
    return uxTaskGetStackHighWaterMark(host_task_h);
}

#ifdef ESP_PLATFORM
#include "esp_bt.h"

#ifdef CONFIG_BT_NIMBLE_HOST_TASK_PRIORITY
# define NIMBLE_HOST_TASK_PRIORITY (CONFIG_BT_NIMBLE_HOST_TASK_PRIORITY)
#else
# define NIMBLE_HOST_TASK_PRIORITY (configMAX_PRIORITIES - 4)
#endif

/**
 * @brief esp_nimble_enable - Initialize the NimBLE host
 *
 * @param host_task
 * @return esp_err_t
 */
esp_err_t esp_nimble_enable(void *host_task)
{
    /*
    * Create task where NimBLE host will run. It is not strictly necessary to
    * have separate task for NimBLE host, but since something needs to handle
    * default queue it is just easier to make separate task which does this.
    */
    xTaskCreatePinnedToCore(host_task, "nimble_host", NIMBLE_HS_STACK_SIZE,
                            NULL, NIMBLE_HOST_TASK_PRIORITY, &host_task_h, NIMBLE_CORE);
    return ESP_OK;

}

/**
 * @brief esp_nimble_disable - Disable the NimBLE host
 *
 * @return esp_err_t
 */
esp_err_t esp_nimble_disable(void)
{
    if (host_task_h) {
        vTaskDelete(host_task_h);
        host_task_h = NULL;
    }
    return ESP_OK;
}


/**
 * @brief nimble_port_freertos_init - Adapt to native nimble api
 *
 * @param host_task_fn
 */
void
nimble_port_freertos_init(TaskFunction_t host_task_fn)
{
    esp_nimble_enable(host_task_fn);
}

/**
 * @brief nimble_port_freertos_deinit - Adapt to native nimble api
 *
 */
void
nimble_port_freertos_deinit(void)
{
    esp_nimble_disable();
}

#else // ESP_PLATFORM

#if NIMBLE_CFG_CONTROLLER
# ifdef CONFIG_BT_NIMBLE_LL_TASK_STACK_SIZE
#  define NIMBLE_LL_STACK_SIZE   (CONFIG_BT_NIMBLE_LL_TASK_STACK_SIZE / 4)
# else
#  if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#   define NIMBLE_LL_STACK_SIZE   (130)
#  else
#   define NIMBLE_LL_STACK_SIZE   (100)
#  endif
# endif

// configMAX_PRIORITIES - 1 is Tmr builtin task
#ifdef CONFIG_BT_NIMBLE_LL_TASK_PRIORITY
# define NIMBLE_LL_TASK_PRIORITY (CONFIG_BT_NIMBLE_LL_TASK_PRIORITY)
#else
# define NIMBLE_LL_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#endif

static StackType_t ll_stack[ NIMBLE_LL_STACK_SIZE ];
static StaticTask_t ll_task_buffer;
static TaskHandle_t ll_task_h;
#endif // NIMBLE_CFG_CONTROLLER

#ifdef CONFIG_BT_NIMBLE_HOST_TASK_PRIORITY
# define NIMBLE_HOST_TASK_PRIORITY (CONFIG_BT_NIMBLE_HOST_TASK_PRIORITY)
#else
# define NIMBLE_HOST_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#endif

static StackType_t hs_stack[ NIMBLE_HS_STACK_SIZE ];
static StaticTask_t hs_task_buffer;

void
nimble_port_freertos_init(TaskFunction_t host_task_fn)
{
#if NIMBLE_CFG_CONTROLLER
    /*
    * Create task where NimBLE LL will run. This one is required as LL has its
    * own event queue and should have highest priority. The task function is
    * provided by NimBLE and in case of FreeRTOS it does not need to be wrapped
    * since it has compatible prototype.
    */
    ll_task_h = xTaskCreateStatic(nimble_port_ll_task_func, "ll", NIMBLE_LL_STACK_SIZE,
                                NULL, NIMBLE_LL_TASK_PRIORITY, ll_stack, &ll_task_buffer);
#endif
    /*
    * Create task where NimBLE host will run. It is not strictly necessary to
    * have separate task for NimBLE host, but since something needs to handle
    * default queue it is just easier to make separate task which does this.
    */
    host_task_h = xTaskCreateStatic(host_task_fn, "host", NIMBLE_HS_STACK_SIZE,
                                    NULL, NIMBLE_HOST_TASK_PRIORITY, hs_stack, &hs_task_buffer);
}

void
nimble_port_freertos_deinit(void)
{
    if (host_task_h) {
        vTaskDelete(host_task_h);
    }
}

#if NIMBLE_CFG_CONTROLLER
UBaseType_t
nimble_port_freertos_get_ll_hwm(void)
{
    if (!ll_task_h) {
        return 0;
    }
    return uxTaskGetStackHighWaterMark(ll_task_h);
}
#endif

#endif // ESP_PLATFORM
