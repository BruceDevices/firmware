/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_MSG_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_MSG_H_

#define PDU_TYPE(data)     (data[0] & BIT_MASK(6))
#define CFG_FILTER_SET     0x00
#define CFG_FILTER_ADD     0x01
#define CFG_FILTER_REMOVE  0x02
#define CFG_FILTER_STATUS  0x03

#define BT_MESH_PROXY_NET_PDU   0x00
#define BT_MESH_PROXY_BEACON    0x01
#define BT_MESH_PROXY_CONFIG    0x02
#define BT_MESH_PROXY_PROV      0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

struct bt_mesh_proxy_role;

typedef int (*proxy_send_cb_t)(uint16_t conn_handle,
	const void *data, uint16_t len);

typedef void (*proxy_recv_cb_t)(struct bt_mesh_proxy_role *role);

struct bt_mesh_proxy_role {
	uint16_t conn_handle;
	uint8_t msg_type;

	struct {
		proxy_send_cb_t send;
		proxy_recv_cb_t recv;
	} cb;

	struct k_work_delayable sar_timer;
	struct os_mbuf *buf;
};

struct bt_mesh_proxy_client {
	struct bt_mesh_proxy_role *cli;
	uint16_t conn_handle;
	uint16_t filter[MYNEWT_VAL(BLE_MESH_PROXY_FILTER_SIZE)];
	enum __packed {
		NONE,
		ACCEPT,
		REJECT,
		} filter_type;
	struct ble_npl_callout send_beacons;
};

int bt_mesh_proxy_msg_recv(struct bt_mesh_proxy_role *role,
	const void *buf, uint16_t len);
int bt_mesh_proxy_msg_send(struct bt_mesh_proxy_role *role, uint8_t type, struct os_mbuf *msg);
void bt_mesh_proxy_msg_init(struct bt_mesh_proxy_role *role);
void bt_mesh_proxy_role_cleanup(struct bt_mesh_proxy_role *role);
struct bt_mesh_proxy_role *bt_mesh_proxy_role_setup(uint16_t conn_handle,
						    proxy_send_cb_t send,
						    proxy_recv_cb_t recv);
struct bt_mesh_proxy_client *find_client(uint16_t conn_handle);
#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_PROXY_MSG_H_ */
