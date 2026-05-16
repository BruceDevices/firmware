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

#ifndef H_BLE_MONITOR_
#define H_BLE_MONITOR_

#include <nimble/porting/nimble/include/syscfg/syscfg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_MONITOR     (MYNEWT_VAL(BLE_MONITOR_RTT) || \
                         MYNEWT_VAL(BLE_MONITOR_UART))

#if BLE_MONITOR
int ble_monitor_out(int c);
int ble_monitor_log(int level, const char *fmt, ...);
#else
static inline int
ble_monitor_out(int c)
{
    (void)c;
    return 0;
}
static inline int
ble_monitor_log(int level, const char *fmt, ...)
{
    (void)level;
    (void)fmt;
    return 0;
}

static inline int
ble_transport_to_ll_cmd(void *buf)
{
    return ble_transport_to_ll_cmd_impl(buf);
}

static inline int
ble_transport_to_ll_acl(struct os_mbuf *om)
{
    return ble_transport_to_ll_acl_impl(om);
}

static inline int
ble_transport_to_ll_iso(struct os_mbuf *om)
{
    return ble_transport_to_ll_iso_impl(om);
}

static inline int
ble_transport_to_hs_evt(void *buf)
{
    return ble_transport_to_hs_evt_impl(buf);
}

static inline int
ble_transport_to_hs_acl(struct os_mbuf *om)
{
    return ble_transport_to_hs_acl_impl(om);
}

static inline int
ble_transport_to_hs_iso(struct os_mbuf *om)
{
    return ble_transport_to_hs_iso_impl(om);
}
#endif /* BLE_MONITOR */

#ifdef __cplusplus
}
#endif

#endif
