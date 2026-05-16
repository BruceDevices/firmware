/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PB_GATT_SRV_H__
#define __PB_GATT_SRV_H__

int bt_mesh_pb_gatt_send(uint16_t conn_handle, struct os_mbuf *buf);

int bt_mesh_pb_gatt_enable(void);
int bt_mesh_pb_gatt_disable(void);

int prov_ccc_write(uint16_t conn_handle, uint8_t type);
void gatt_disconnected_pb_gatt(struct ble_gap_conn_desc conn, uint8_t err);
void gatt_connected_pb_gatt(uint16_t conn_handle, uint8_t err);
void resolve_svc_handles(void);

int bt_mesh_pb_gatt_adv_start(void);

extern struct svc_handles {
	uint16_t proxy_h;
	uint16_t proxy_data_out_h;
	uint16_t prov_h;
	uint16_t prov_data_in_h;
	uint16_t prov_data_out_h;
} svc_handles;

#endif /* __PB_GATT_SRV_H__ */
