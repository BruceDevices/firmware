/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_PROXY_LOG

#include "nimble/nimble/host/mesh/include/mesh/slist.h"
#include "nimble/nimble/host/mesh/include/mesh/mesh.h"
#include "nimble/nimble/host/src/ble_hs_priv.h"
#include "nimble/nimble/host/services/gatt/include/services/gatt/ble_svc_gatt.h"

#include "mesh_priv.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#if defined(CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME)
#define ADV_OPT_USE_NAME BT_LE_ADV_OPT_USE_NAME
#else
#define ADV_OPT_USE_NAME 0
#endif

#define ADV_OPT_PROXY                                                           \
.conn_mode = (BLE_GAP_CONN_MODE_UND),                                  \
.disc_mode = (BLE_GAP_DISC_MODE_GEN),


#define BT_UUID_MESH_PROXY_VAL            0x1828
#define CLIENT_BUF_SIZE 66

#define BT_UUID_16_ENCODE(w16)  \
	(((w16) >>  0) & 0xFF), \
	(((w16) >>  8) & 0xFF)

static sys_slist_t idle_waiters;
static atomic_t pending_notifications;

static void proxy_send_beacons(struct ble_npl_event *work);

static int proxy_send(uint16_t conn_handle,
	const void *data, uint16_t len);


static struct bt_mesh_proxy_client clients[CONFIG_BT_MAX_CONN];

static bool service_registered;

static int conn_count;

struct bt_mesh_proxy_client *find_client(uint16_t conn_handle)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn_handle == conn_handle) {
			return &clients[i];
		}
	}
	return NULL;
}

static struct bt_mesh_proxy_client *get_client(uint16_t conn_handle)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].conn_handle == 0xffff) {
			clients[i].conn_handle = conn_handle;
			return &clients[i];
		}
	}
	return NULL;
}

/* Next subnet in queue to be advertised */
static struct bt_mesh_subnet *beacon_sub;

static int filter_set(struct bt_mesh_proxy_client *client,
		      struct os_mbuf *buf)
{
	uint8_t type;

	if (buf->om_len < 1) {
		BT_WARN("Too short Filter Set message");
		return -EINVAL;
	}

	type = net_buf_simple_pull_u8(buf);
	BT_DBG("type 0x%02x", type);

	switch (type) {
		case 0x00:
			(void)memset(client->filter, 0, sizeof(client->filter));
			client->filter_type = ACCEPT;
			break;
		case 0x01:
			(void)memset(client->filter, 0, sizeof(client->filter));
			client->filter_type = REJECT;
			break;
		default:
			BT_WARN("Prohibited Filter Type 0x%02x", type);
			return -EINVAL;
	}

	return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	BT_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == BT_MESH_ADDR_UNASSIGNED) {
			client->filter[i] = addr;
			return;
		}
	}
}

static void filter_remove(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	BT_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			client->filter[i] = BT_MESH_ADDR_UNASSIGNED;
			return;
		}
	}
}

static void send_filter_status(struct bt_mesh_proxy_client *client,
			       struct bt_mesh_net_rx *rx,
			       struct os_mbuf *buf)
{
	struct bt_mesh_net_tx tx = {
		.sub = rx->sub,
		.ctx = &rx->ctx,
		.src = bt_mesh_primary_addr(),
	};
	uint16_t filter_size;
	int i, err;

	/* Configuration messages always have dst unassigned */
	tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

	net_buf_simple_init(buf, 10);

	net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

	if (client->filter_type == ACCEPT) {
		net_buf_simple_add_u8(buf, 0x00);
	} else {
		net_buf_simple_add_u8(buf, 0x01);
	}

	for (filter_size = 0U, i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
			filter_size++;
		}
	}

	net_buf_simple_add_be16(buf, filter_size);

	BT_DBG("%u bytes: %s", buf->om_len, bt_hex(buf->om_data, buf->om_len));

	err = bt_mesh_net_encode(&tx, buf, true);
	if (err) {
		BT_ERR("Encoding Proxy cfg message failed (err %d)", err);
		return;
	}

	err = bt_mesh_proxy_msg_send(client->cli, BT_MESH_PROXY_CONFIG, buf);
	if (err) {
		BT_ERR("Failed to send proxy cfg message (err %d)", err);
	}
}

static void proxy_filter_recv(uint16_t conn_handle,
			      struct bt_mesh_net_rx *rx, struct os_mbuf *buf)
{
	struct bt_mesh_proxy_client *client;
	uint8_t opcode;

	client = find_client(conn_handle);

	opcode = net_buf_simple_pull_u8(buf);
	switch (opcode) {
	case CFG_FILTER_SET:
		filter_set(client, buf);
		send_filter_status(client, rx, buf);
		break;
		case CFG_FILTER_ADD:
			while (buf->om_len >= 2) {
				uint16_t addr;

				addr = net_buf_simple_pull_be16(buf);
				filter_add(client, addr);
			}
			send_filter_status(client, rx, buf);
			break;
		case CFG_FILTER_REMOVE:
			while (buf->om_len >= 2) {
				uint16_t addr;

				addr = net_buf_simple_pull_be16(buf);
				filter_remove(client, addr);
			}
			send_filter_status(client, rx, buf);
			break;
		default:
			BT_WARN("Unhandled configuration OpCode 0x%02x", opcode);
			break;
	}
}

static void proxy_cfg(struct bt_mesh_proxy_role *role)
{
	struct os_mbuf *buf = NET_BUF_SIMPLE(BT_MESH_NET_MAX_PDU_LEN);
	struct bt_mesh_net_rx rx;
	int err;

	err = bt_mesh_net_decode(role->buf, BT_MESH_NET_IF_PROXY_CFG,
				 &rx, buf);
	if (err) {
		BT_ERR("Failed to decode Proxy Configuration (err %d)", err);
		return;
	}

	rx.local_match = 1U;

	if (bt_mesh_rpl_check(&rx, NULL)) {
		BT_WARN("Replay: src 0x%04x dst 0x%04x seq 0x%06" PRIx32,
			rx.ctx.addr, rx.ctx.recv_dst, rx.seq);
		return;
	}

	/* Remove network headers */
	net_buf_simple_pull(buf, BT_MESH_NET_HDR_LEN);

	BT_DBG("%u bytes: %s", buf->om_len, bt_hex(buf->om_data, buf->om_len));

	if (buf->om_len < 1) {
		BT_WARN("Too short proxy configuration PDU");
		return;
	}

	proxy_filter_recv(role->conn_handle, &rx, buf);
}

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_NET_PDU:
		BT_DBG("Mesh Network PDU");
		bt_mesh_net_recv(role->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		BT_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(role->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		BT_DBG("Mesh Configuration PDU");
		proxy_cfg(role);
		break;
	default:
		BT_WARN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static int beacon_send(struct bt_mesh_proxy_client *client, struct bt_mesh_subnet *sub)
{
	struct os_mbuf *buf = NET_BUF_SIMPLE(23);
	int rc;

	net_buf_simple_init(buf, 1);
	bt_mesh_beacon_create(sub, buf);

	rc = bt_mesh_proxy_msg_send(client->cli, BT_MESH_PROXY_BEACON, buf);
	os_mbuf_free_chain(buf);
	return rc;
}

static int send_beacon_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	struct bt_mesh_proxy_client *client = cb_data;

	return beacon_send(client, sub);
}

static void proxy_send_beacons(struct ble_npl_event *work)
{
	struct bt_mesh_proxy_client *client;

	client = ble_npl_event_get_arg(work);

	(void)bt_mesh_subnet_find(send_beacon_cb, client);
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
	int i;

	if (!sub) {
		/* NULL means we send on all subnets */
		bt_mesh_subnet_foreach(bt_mesh_proxy_beacon_send);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			beacon_send(&clients[i], sub);
		}
	}
}

static void node_id_start(struct bt_mesh_subnet *sub)
{
	sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;
	sub->node_id_start = k_uptime_get_32();
}

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub)
{
	node_id_start(sub);

	/* Prioritize the recently enabled subnet */
	beacon_sub = sub;
}

void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub)
	{
	sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
	sub->node_id_start = 0U;
}

int bt_mesh_proxy_identity_enable(void)
{
	BT_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (bt_mesh_subnet_foreach(node_id_start)) {
		bt_mesh_adv_update();
	}

	return 0;
}

#define ID_TYPE_NET  0x00
#define ID_TYPE_NODE 0x01

#define NODE_ID_LEN  19
#define NET_ID_LEN   11

#define NODE_ID_TIMEOUT (CONFIG_BT_MESH_NODE_ID_TIMEOUT * MSEC_PER_SEC)

static uint8_t proxy_svc_data[NODE_ID_LEN] = {
	BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL),
};

static const struct bt_data node_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
		      BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NODE_ID_LEN),
};

static const struct bt_data net_id_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
		      BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NET_ID_LEN),
};

static int node_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	struct ble_gap_adv_params fast_adv_param = {
		ADV_OPT_PROXY
		ADV_FAST_INT
	};
#if ADV_OPT_USE_NAME
	const char *name = CONFIG_BT_DEVICE_NAME;
	size_t name_len = strlen(name);
	struct bt_data sd = {
		.type = BT_DATA_NAME_COMPLETE,
		.data_len = name_len,
		.data = (void *)name
	};
#else
	struct bt_data *sd = NULL;
#endif
	uint8_t tmp[16];
	int err;

	BT_DBG("");

	proxy_svc_data[2] = ID_TYPE_NODE;

	err = bt_rand(proxy_svc_data + 11, 8);
	if (err) {
		return err;
	}

	(void)memset(tmp, 0, 6);
	memcpy(tmp + 6, proxy_svc_data + 11, 8);
	sys_put_be16(bt_mesh_primary_addr(), tmp + 14);

	err = bt_encrypt_be(sub->keys[SUBNET_KEY_TX_IDX(sub)].identity, tmp,
			    tmp);
	if (err) {
		return err;
	}

	memcpy(proxy_svc_data + 3, tmp + 8, 8);

	err = bt_mesh_adv_start(&fast_adv_param, duration, node_id_ad,
			      ARRAY_SIZE(node_id_ad), sd, 0);
	if (err) {
		BT_WARN("Failed to advertise using Node ID (err %d)", err);
		return err;
	}

	return 0;
}

static int net_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	struct ble_gap_adv_params slow_adv_param = {
		ADV_OPT_PROXY
		ADV_SLOW_INT
	};
#if ADV_OPT_USE_NAME
	const char *name = CONFIG_BT_DEVICE_NAME;
	size_t name_len = strlen(name);
	struct bt_data sd = {
		.type = BT_DATA_NAME_COMPLETE,
		.data_len = name_len,
		.data = (void *)name
	};
#else
	struct bt_data *sd = NULL;
#endif
	int err;

	BT_DBG("");

	proxy_svc_data[2] = ID_TYPE_NET;

	BT_DBG("Advertising with NetId %s",
	       bt_hex(sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8));

	memcpy(proxy_svc_data + 3, sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8);

	err = bt_mesh_adv_start(&slow_adv_param, duration, net_id_ad,
			      ARRAY_SIZE(net_id_ad), sd, 0);
	if (err) {
		BT_WARN("Failed to advertise using Network ID (err %d)", err);
		return err;
	}

	return 0;
}

static bool advertise_subnet(struct bt_mesh_subnet *sub)
{
	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return false;
	}

	return (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING ||
	bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED);
}

static struct bt_mesh_subnet *next_sub(void)
{
	struct bt_mesh_subnet *sub = NULL;

	if (!beacon_sub) {
		beacon_sub = bt_mesh_subnet_next(NULL);
		if (!beacon_sub) {
			/* No valid subnets */
			return NULL;
		}
	}

	sub = beacon_sub;
	do {
		if (advertise_subnet(sub)) {
			beacon_sub = sub;
			return sub;
		}

		sub = bt_mesh_subnet_next(sub);
	} while (sub != beacon_sub);

	/* No subnets to advertise on */
	return NULL;
}

static int sub_count_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	int *count = cb_data;

	if (advertise_subnet(sub)) {
		(*count)++;
	}

	return 0;
}

static int sub_count(void)
{
	int count = 0;

	(void)bt_mesh_subnet_find(sub_count_cb, &count);

	return count;
}

static int gatt_proxy_advertise(struct bt_mesh_subnet *sub)
{
	int32_t remaining = K_FOREVER;
	int subnet_count;
	int err = -EBUSY;

	BT_DBG("");

	if (conn_count == CONFIG_BT_MAX_CONN) {
		BT_DBG("Connectable advertising deferred (max connections %d)", conn_count);
		return -ENOMEM;
	}

	sub = beacon_sub ? beacon_sub : bt_mesh_subnet_next(beacon_sub);
	if (!sub) {
		BT_WARN("No subnets to advertise on");
		return -ENOENT;
	}

	subnet_count = sub_count();
	BT_DBG("sub_count %u", subnet_count);
	if (subnet_count > 1) {
		int32_t max_timeout;

		/* We use NODE_ID_TIMEOUT as a starting point since it may
		 * be less than 60 seconds. Divide this period into at least
		 * 6 slices, but make sure that a slice is at least one
		 * second long (to avoid excessive rotation).
		 */
		max_timeout = NODE_ID_TIMEOUT / MAX(subnet_count, 6);
		max_timeout = MAX(max_timeout, K_SECONDS(1));

		if (remaining > max_timeout || remaining < 0) {
			remaining = max_timeout;
		}
	}

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		uint32_t active = k_uptime_get_32() - sub->node_id_start;

		if (active < NODE_ID_TIMEOUT) {
			remaining = NODE_ID_TIMEOUT - active;
			BT_DBG("Node ID active for %u ms, %d ms remaining",
			       active, remaining);
			err = node_id_adv(sub, remaining);
		} else {
			bt_mesh_proxy_identity_stop(sub);
			BT_DBG("Node ID stopped");
		}
	}

	if (sub->node_id == BT_MESH_NODE_IDENTITY_STOPPED) {
		err = net_id_adv(sub, remaining);
	}

	BT_DBG("Advertising %d ms for net_idx 0x%04x",
	       (int) remaining, sub->net_idx);

	beacon_sub = bt_mesh_subnet_next(beacon_sub);

	return err;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt == BT_MESH_KEY_DELETED) {
		if (sub == beacon_sub) {
			beacon_sub = NULL;
		}
	} else {
		bt_mesh_proxy_beacon_send(sub);
	}
}

static void proxy_ccc_write(uint16_t conn_handle)
{
	struct bt_mesh_proxy_client *client;

	BT_DBG("conn_handle %d", conn_handle);

	client = find_client(conn_handle);

	if (client->filter_type == NONE) {
		client->filter_type = ACCEPT;
		k_work_add_arg(&client->send_beacons, client);
		k_work_submit(&client->send_beacons);
	}
}

int bt_mesh_proxy_gatt_enable(void)
{
	uint16_t handle;
	int rc;
	int i;

	BT_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_VAL), &handle);
	assert(rc == 0);
	ble_gatts_svc_set_visibility(handle, 1);
	/* FIXME: figure out end handle */
	ble_svc_gatt_changed(svc_handles.proxy_h, 0xffff);

	service_registered = true;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			clients[i].filter_type = ACCEPT;
		}
	}
	return 0;
}

void bt_mesh_proxy_gatt_disconnect(void)
{
	int rc;
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if ((client->cli) &&
		    (client->filter_type == ACCEPT ||
		    client->filter_type == REJECT)) {
			client->filter_type = NONE;
			rc = ble_gap_terminate(client->cli->conn_handle,
			                       BLE_ERR_REM_USER_CONN_TERM);
			assert(rc == 0);
		}
	}
}

int bt_mesh_proxy_gatt_disable(void)
{
	uint16_t handle;
	int rc;
	BT_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	bt_mesh_proxy_gatt_disconnect();

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_VAL), &handle);
	assert(rc == 0);
	ble_gatts_svc_set_visibility(handle, 0);
	/* FIXME: figure out end handle */
	ble_svc_gatt_changed(svc_handles.proxy_h, 0xffff);
	service_registered = false;

	return 0;
}

void bt_mesh_proxy_addr_add(struct os_mbuf *buf, uint16_t addr)
{
	struct bt_mesh_proxy_client *client;
	struct bt_mesh_proxy_role *cli =
		CONTAINER_OF(buf, struct bt_mesh_proxy_role, buf);

	client = find_client(cli->conn_handle);

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == ACCEPT) {
		filter_add(client, addr);
	} else if (client->filter_type == REJECT) {
		filter_remove(client, addr);
	}
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
				uint16_t addr)
{
	int i;

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == REJECT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return false;
			}
		}

		return true;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		return true;
	}

	if (client->filter_type == ACCEPT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return true;
			}
		}
	}

	return false;
}

bool bt_mesh_proxy_relay(struct os_mbuf *buf, uint16_t dst)
{
	const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
	void *cb_data = BT_MESH_ADV(buf)->cb_data;
	bool relayed = false;
	int i, err;

	BT_DBG("%u bytes to dst 0x%04x", buf->om_len, dst);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];
		struct os_mbuf *msg;

		if (!client->cli) {
			continue;
		}

		if (!client_filter_match(client, dst)) {
			continue;
		}

		/* Proxy PDU sending modifies the original buffer,
		 * so we need to make a copy.
		 */
		msg = NET_BUF_SIMPLE(32);
		net_buf_simple_init(msg, 1);
		net_buf_simple_add_mem(msg, buf->om_data, buf->om_len);

		err = bt_mesh_proxy_msg_send(client->cli, BT_MESH_PROXY_NET_PDU,
					     msg);

		adv_send_start(0, err, cb, cb_data);
		if (err) {
			BT_ERR("Failed to send proxy message (err %d)", err);

			/* If segment_and_send() fails the buf_send_end() callback will
			 * not be called, so we need to clear the user data (net_buf,
			 * which is just opaque data to segment_and send) reference given
			 * to segment_and_send() here.
			 */
			net_buf_unref(buf);
			continue;
		}
		os_mbuf_free_chain(msg);
		relayed = true;
	}

	return relayed;
}

static void gatt_connected(uint16_t conn_handle)
{
	struct bt_mesh_proxy_client *client;
	struct ble_gap_conn_desc info;
	struct ble_hs_conn *conn;

	conn = ble_hs_conn_find(conn_handle);
	bt_conn_get_info(conn, &info);
	if (info.role != BLE_GAP_ROLE_SLAVE ||
	    !service_registered) {
		return;
	}
	BT_DBG("conn %d", conn_handle);

	conn_count++;

	client = get_client(conn_handle);
	assert(client);

	client->filter_type = NONE;
	(void)memset(client->filter, 0, sizeof(client->filter));

	client->cli = bt_mesh_proxy_role_setup(conn_handle, proxy_send,
					       proxy_msg_recv);

	/* Try to re-enable advertising in case it's possible */
	if (conn_count < CONFIG_BT_MAX_CONN) {
		bt_mesh_adv_update();
	}
}

static void gatt_disconnected(struct ble_gap_conn_desc conn, uint8_t reason)
{
	struct bt_mesh_proxy_client *client;

	if (conn.role != BLE_GAP_ROLE_SLAVE) {
		return;
	}

	if (!service_registered && bt_mesh_is_provisioned()) {
		(void)bt_mesh_proxy_gatt_enable();
		return;
	}

	conn_count--;
	client = find_client(conn.conn_handle);
	if (client->cli) {
		bt_mesh_proxy_role_cleanup(client->cli);
		client->cli = NULL;
	}

	client->conn_handle = 0xffff;
}

void notify_complete(void)
{
	sys_snode_t *n;

	if (atomic_dec(&pending_notifications) > 1) {
		return;
	}

	BT_DBG("");

	while ((n = sys_slist_get(&idle_waiters))) {
		CONTAINER_OF(n, struct bt_mesh_proxy_idle_cb, n)->cb();
	}
}

static int proxy_send(uint16_t conn_handle,
		      const void *data, uint16_t len)
{
	struct os_mbuf *om;
	int err = 0;

	BT_DBG("%u bytes: %s", len, bt_hex(data, len));

	om = ble_hs_mbuf_from_flat(data, len);
	assert(om);
	err = ble_gatts_notify_custom(conn_handle, svc_handles.proxy_data_out_h, om);
	notify_complete();

	if (!err) {
		atomic_inc(&pending_notifications);
	}

	return err;
}

int bt_mesh_proxy_adv_start(void)
{
	BT_DBG("");

	if (!service_registered || !bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	return gatt_proxy_advertise(next_sub());
}


static void ble_mesh_handle_connect(struct ble_gap_event *event, void *arg)
{
#if MYNEWT_VAL(BLE_EXT_ADV)
	/* When EXT ADV is enabled then mesh proxy is connected
	 * when proxy advertising instance is completed.
	 * Therefore no need to handle BLE_GAP_EVENT_CONNECT
	 */
	if (event->type == BLE_GAP_EVENT_ADV_COMPLETE) {
		/* Reason 0 means advertising has been completed because
		 * connection has been established
		 */
		if (event->adv_complete.reason != 0) {
			return;
		}

#if MYNEWT_VAL(BLE_MESH_PROXY)
		if (event->adv_complete.instance != BT_MESH_ADV_GATT_INST) {
			return;
		}
#endif

		gatt_connected(event->adv_complete.conn_handle);
#if MYNEWT_VAL(BLE_MESH_PB_GATT)
		gatt_connected_pb_gatt(event->adv_complete.conn_handle,
				       event->adv_complete.reason);
#endif
	}
#else
	if (event->type == BLE_GAP_EVENT_CONNECT) {
		gatt_connected(event->connect.conn_handle);
#if MYNEWT_VAL(BLE_MESH_PB_GATT)
		gatt_connected_pb_gatt(event->connect.conn_handle, event->connect.status);
#endif
	}
#endif
}

int ble_mesh_proxy_gap_event(struct ble_gap_event *event, void *arg)
{
	if ((event->type == BLE_GAP_EVENT_CONNECT) ||
	    (event->type == BLE_GAP_EVENT_ADV_COMPLETE)) {
		ble_mesh_handle_connect(event, arg);
	} else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
		gatt_disconnected(event->disconnect.conn,
				   event->disconnect.reason);
#if MYNEWT_VAL(BLE_MESH_PB_GATT)
		gatt_disconnected_pb_gatt(event->disconnect.conn,
				  event->disconnect.reason);
#endif
	} else if (event->type == BLE_GAP_EVENT_SUBSCRIBE) {
		if (event->subscribe.attr_handle == svc_handles.proxy_data_out_h) {
#if (MYNEWT_VAL(BLE_MESH_GATT_PROXY))
			proxy_ccc_write(event->subscribe.conn_handle);
#endif
		} else if (event->subscribe.attr_handle ==
			   svc_handles.prov_data_out_h) {
#if (MYNEWT_VAL(BLE_MESH_PB_GATT))
			prov_ccc_write(event->subscribe.conn_handle, event->type);
#endif
		}
	}

	return 0;
}

int bt_mesh_proxy_init(void)
{
	int i;

#if (MYNEWT_VAL(BLE_MESH_GATT_PROXY))
	if (!bt_mesh_subnet_cb_list[4]) {
		bt_mesh_subnet_cb_list[4] = subnet_evt;
	}
#endif

	for (i = 0; i < MYNEWT_VAL(BLE_MAX_CONNECTIONS); ++i) {
#if (MYNEWT_VAL(BLE_MESH_GATT_PROXY))
		k_work_init(&clients[i].send_beacons, proxy_send_beacons);
#endif
		clients[i].conn_handle = 0xffff;
	}

	resolve_svc_handles();

	ble_gatts_svc_set_visibility(svc_handles.proxy_h, 0);
	ble_gatts_svc_set_visibility(svc_handles.prov_h, 0);

	return 0;
}

#endif /* MYNEWT_VAL(BLE_MESH) */
