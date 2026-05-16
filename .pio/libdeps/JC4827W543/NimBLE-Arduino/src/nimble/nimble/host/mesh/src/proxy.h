/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PROXY_H__
#define __PROXY_H__

#include "nimble/nimble/host/mesh/include/mesh/slist.h"

#if CONFIG_BT_MESH_DEBUG_USE_ID_ADDR
#define ADV_OPT_USE_IDENTITY BT_LE_ADV_OPT_USE_IDENTITY
#else
#define ADV_OPT_USE_IDENTITY 0
#endif

#define ADV_SLOW_INT                                                           \
.itvl_min = BT_GAP_ADV_SLOW_INT_MIN,                             \
.itvl_max = BT_GAP_ADV_SLOW_INT_MAX,

#define ADV_FAST_INT                                                           \
.itvl_min = BT_GAP_ADV_FAST_INT_MIN_2,                             \
.itvl_max = BT_GAP_ADV_FAST_INT_MAX_2,

struct bt_mesh_proxy_idle_cb {
	sys_snode_t n;
	void (*cb)(void);
};

void notify_complete(void);
int bt_mesh_proxy_gatt_enable(void);
int bt_mesh_proxy_gatt_disable(void);
void bt_mesh_proxy_gatt_disconnect(void);

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub);

int bt_mesh_proxy_adv_start(void);

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub);
void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub);

bool bt_mesh_proxy_relay(struct os_mbuf *buf, uint16_t dst);
void bt_mesh_proxy_addr_add(struct os_mbuf *buf, uint16_t addr);

int ble_mesh_proxy_gap_event(struct ble_gap_event *event, void *arg);
int bt_mesh_proxy_init(void);

#endif /* __PROXY_H__ */
