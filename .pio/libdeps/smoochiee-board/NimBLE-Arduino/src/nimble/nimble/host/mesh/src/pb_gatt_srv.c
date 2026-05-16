/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MESH_LOG_MODULE BLE_MESH_PROV_LOG

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
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/nimble/host/services/gatt/include/services/gatt/ble_svc_gatt.h"
#include "nimble/nimble/host/src/ble_hs_priv.h"

#if defined(CONFIG_BT_MESH_PB_GATT_USE_DEVICE_NAME)
#define ADV_OPT_USE_NAME BT_LE_ADV_OPT_USE_NAME
#else
#define ADV_OPT_USE_NAME 0
#endif

#define ADV_OPT_PROV                                                           \
.conn_mode = (BLE_GAP_CONN_MODE_UND),                                  \
.disc_mode = (BLE_GAP_DISC_MODE_GEN),

#if MYNEWT_VAL(BLE_MESH_PB_GATT)
/** @def BT_UUID_MESH_PROV
 *  @brief Mesh Provisioning Service
 */
ble_uuid16_t BT_UUID_MESH_PROV                 = BLE_UUID16_INIT(0x1827);
#define BT_UUID_MESH_PROV_VAL             0x1827
/** @def BT_UUID_MESH_PROXY
 *  @brief Mesh Proxy Service
 */
ble_uuid16_t BT_UUID_MESH_PROXY                = BLE_UUID16_INIT(0x1828);
#define BT_UUID_MESH_PROXY_VAL            0x1828
/** @def BT_UUID_GATT_CCC
 *  @brief GATT Client Characteristic Configuration
 */
ble_uuid16_t BT_UUID_GATT_CCC                  = BLE_UUID16_INIT(0x2902);
#define BT_UUID_GATT_CCC_VAL              0x2902
/** @def BT_UUID_MESH_PROV_DATA_IN
 *  @brief Mesh Provisioning Data In
 */
ble_uuid16_t BT_UUID_MESH_PROV_DATA_IN         = BLE_UUID16_INIT(0x2adb);
#define BT_UUID_MESH_PROV_DATA_IN_VAL     0x2adb
/** @def BT_UUID_MESH_PROV_DATA_OUT
 *  @brief Mesh Provisioning Data Out
 */
ble_uuid16_t BT_UUID_MESH_PROV_DATA_OUT        = BLE_UUID16_INIT(0x2adc);
#define BT_UUID_MESH_PROV_DATA_OUT_VAL    0x2adc
/** @def BT_UUID_MESH_PROXY_DATA_IN
 *  @brief Mesh Proxy Data In
 */
ble_uuid16_t BT_UUID_MESH_PROXY_DATA_IN        = BLE_UUID16_INIT(0x2add);
#define BT_UUID_MESH_PROXY_DATA_IN_VAL    0x2add
/** @def BT_UUID_MESH_PROXY_DATA_OUT
 *  @brief Mesh Proxy Data Out
 */
ble_uuid16_t BT_UUID_MESH_PROXY_DATA_OUT       = BLE_UUID16_INIT(0x2ade);
#define BT_UUID_MESH_PROXY_DATA_OUT_VAL   0x2ade
#define BT_UUID_16_ENCODE(w16)  \
	(((w16) >>  0) & 0xFF), \
	(((w16) >>  8) & 0xFF)

static bool prov_fast_adv;

struct svc_handles svc_handles;
static atomic_t pending_notifications;

static int gatt_send(uint16_t conn_handle,
		     const void *data, uint16_t len);

static struct bt_mesh_proxy_role *cli;

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_PROV:
		BT_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(role->conn_handle, role->buf);
		break;
	default:
		BT_WARN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static bool service_registered;

static int gatt_recv_proxy(uint16_t conn_handle, uint16_t attr_handle,
			 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const uint8_t *data = ctxt->om->om_data;
	uint16_t len = ctxt->om->om_len;
	struct bt_mesh_proxy_client *client = find_client(conn_handle);

	if (len < 1) {
		BT_WARN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) == BT_MESH_PROXY_PROV) {
		BT_WARN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(client->cli, data, len);
}

static int gatt_recv_prov(uint16_t conn_handle, uint16_t attr_handle,
		     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const uint8_t *data = ctxt->om->om_data;
	uint16_t len = ctxt->om->om_len;

	if (conn_handle != cli->conn_handle) {
		BT_WARN("conn_handle != cli->conn_handle");
		return -ENOTCONN;
	}

	if (len < 1) {
		BT_WARN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) != BT_MESH_PROXY_PROV) {
		BT_WARN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(cli, data, len);
}

void gatt_connected_pb_gatt(uint16_t conn_handle, uint8_t err)
{
	struct ble_gap_conn_desc info;
	struct ble_hs_conn *conn;

	conn = ble_hs_conn_find(conn_handle);
	bt_conn_get_info(conn, &info);
	if (info.role != BLE_GAP_ROLE_SLAVE ||
	    !service_registered || bt_mesh_is_provisioned()) {
		return;
	}

	cli = bt_mesh_proxy_role_setup(conn_handle, gatt_send, proxy_msg_recv);

	BT_DBG("conn %p err 0x%02x", (void *)conn, err);
}

void gatt_disconnected_pb_gatt(struct ble_gap_conn_desc conn, uint8_t reason)
{
	if (conn.role != BLE_GAP_ROLE_SLAVE ||
	    !service_registered) {
		return;
	}

	if (cli) {
		bt_mesh_proxy_role_cleanup(cli);
		cli = NULL;
	}

	BT_DBG("conn_handle %d reason 0x%02x", conn.conn_handle, reason);

	bt_mesh_pb_gatt_close(conn.conn_handle);

	if (bt_mesh_is_provisioned()) {
		(void)bt_mesh_pb_gatt_disable();
	}
}

int prov_ccc_write(uint16_t conn_handle, uint8_t type)
{
	if (cli->conn_handle != conn_handle) {
		BT_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	if (type != BLE_GAP_EVENT_SUBSCRIBE) {
		BT_WARN("Client wrote instead enabling notify");
		return BT_GATT_ERR(EINVAL);
	}

	bt_mesh_pb_gatt_open(conn_handle);

	return 0;
}

/* Mesh Provisioning Service Declaration */

static int
dummy_access_cb(uint16_t conn_handle, uint16_t attr_handle,
		struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	/*
	 * We should never never enter this callback - it's attached to notify-only
	 * characteristic which are notified directly from mbuf. And we can't pass
	 * NULL as access_cb because gatts will assert on init...
	 */
	BLE_HS_DBG_ASSERT(0);
	return 0;
}

static const struct ble_gatt_svc_def svc_defs [] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_VAL),
		.characteristics = (struct ble_gatt_chr_def[]) { {
				.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_DATA_IN_VAL),
				.access_cb = gatt_recv_proxy,
				.flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
			}, {
				.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_DATA_OUT_VAL),
				.access_cb = dummy_access_cb,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			}, {
			0, /* No more characteristics in this service. */
		} },
	}, {
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL),
		.characteristics = (struct ble_gatt_chr_def[]) { {
				.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_DATA_IN_VAL),
				.access_cb = gatt_recv_prov,
				.flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
			}, {
				.uuid = BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_DATA_OUT_VAL),
				.access_cb = dummy_access_cb,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			}, {
				0, /* No more characteristics in this service. */
		} },
	}, {
		0, /* No more services. */
	},
};

void resolve_svc_handles(void)
{
	int rc;

	/* Either all handles are already resolved, or none of them */
	if (svc_handles.prov_data_out_h) {
		return;
	}

	/*
	 * We assert if attribute is not found since at this stage all attributes
	 * shall be already registered and thus shall be found.
	 */

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_VAL),
				&svc_handles.proxy_h);
	assert(rc == 0);

	rc = ble_gatts_find_chr(BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_VAL),
				BLE_UUID16_DECLARE(BT_UUID_MESH_PROXY_DATA_OUT_VAL),
				NULL, &svc_handles.proxy_data_out_h);
	assert(rc == 0);

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL),
				&svc_handles.prov_h);
	assert(rc == 0);

	rc = ble_gatts_find_chr(BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL),
				BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_DATA_IN_VAL),
				NULL, &svc_handles.prov_data_in_h);
	assert(rc == 0);

	rc = ble_gatts_find_chr(BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL),
				BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_DATA_OUT_VAL),
				NULL, &svc_handles.prov_data_out_h);
	assert(rc == 0);
}


int bt_mesh_proxy_svcs_register(void)
{
	int rc;

	rc = ble_gatts_count_cfg(svc_defs);
	assert(rc == 0);

	rc = ble_gatts_add_svcs(svc_defs);
	assert(rc == 0);

	return 0;
}

int bt_mesh_pb_gatt_enable(void)
{
	int rc;
	uint16_t handle;
	BT_DBG("");

	if (bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL), &handle);
	assert(rc == 0);
	ble_gatts_svc_set_visibility(handle, 1);
	/* FIXME: figure out end handle */
	ble_svc_gatt_changed(svc_handles.prov_h, 0xffff);

	service_registered = true;
	prov_fast_adv = true;

	return 0;
}

int bt_mesh_pb_gatt_disable(void)
{
	uint16_t handle;
	int rc;

	BT_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	rc = ble_gatts_find_svc(BLE_UUID16_DECLARE(BT_UUID_MESH_PROV_VAL), &handle);
	assert(rc == 0);
	ble_gatts_svc_set_visibility(handle, 0);
	/* FIXME: figure out end handle */
	ble_svc_gatt_changed(svc_handles.prov_h, 0xffff);
	service_registered = false;

	bt_mesh_adv_update();

	return 0;
}

static uint8_t prov_svc_data[20] = {
	BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL),
};

static const struct bt_data prov_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL)),
		      BT_DATA(BT_DATA_SVC_DATA16, prov_svc_data, sizeof(prov_svc_data)),
};

int bt_mesh_pb_gatt_send(uint16_t conn_handle, struct os_mbuf *buf)
{
	if (!cli || cli->conn_handle != conn_handle) {
		BT_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	return bt_mesh_proxy_msg_send(cli, BT_MESH_PROXY_PROV, buf);
}

static size_t gatt_prov_adv_create(struct bt_data prov_sd[1])
{
	const struct bt_mesh_prov *prov = bt_mesh_prov_get();
	size_t uri_len;

	memcpy(prov_svc_data + 2, prov->uuid, 16);
	sys_put_be16(prov->oob_info, prov_svc_data + 18);

	if (!prov->uri) {
		return 0;
	}

	uri_len = strlen(prov->uri);
	if (uri_len > 29) {
		/* There's no way to shorten an URI */
		BT_WARN("Too long URI to fit advertising packet");
		return 0;
	}

	prov_sd[0].type = BT_DATA_URI;
	prov_sd[0].data_len = uri_len;
	prov_sd[0].data = (const uint8_t *)prov->uri;

	return 1;
}

static int gatt_send(uint16_t conn_handle,
	const void *data, uint16_t len)
{
	struct os_mbuf *om;
	int err = 0;

	BT_DBG("%u bytes: %s", len, bt_hex(data, len));
	om = ble_hs_mbuf_from_flat(data, len);
	assert(om);
	err = ble_gatts_notify_custom(conn_handle, svc_handles.prov_data_out_h, om);
	notify_complete();

	if (!err) {
		atomic_inc(&pending_notifications);
	}

	return err;
}

int bt_mesh_pb_gatt_adv_start(void)
{
	BT_DBG("");

	if (!service_registered || bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	struct ble_gap_adv_params fast_adv_param = {
		ADV_OPT_PROV
		ADV_FAST_INT
	};
#if ADV_OPT_USE_NAME
	const char *name = CONFIG_BT_DEVICE_NAME;
	size_t name_len = strlen(name);
	struct bt_data prov_sd = {
		.type = BT_DATA_NAME_COMPLETE,
		.data_len = name_len,
		.data = (void *)name
	};
#else
	struct bt_data *prov_sd = NULL;
#endif

	size_t prov_sd_len;
	int err;

	prov_sd_len = gatt_prov_adv_create(prov_sd);

	if (!prov_fast_adv) {
		struct ble_gap_adv_params slow_adv_param = {
			ADV_OPT_PROV
			ADV_SLOW_INT
		};

		return bt_mesh_adv_start(&slow_adv_param, K_FOREVER, prov_ad,
					 ARRAY_SIZE(prov_ad), prov_sd, prov_sd_len);
	}

	/* Advertise 60 seconds using fast interval */
	err = bt_mesh_adv_start(&fast_adv_param, (60 * MSEC_PER_SEC),
				prov_ad, ARRAY_SIZE(prov_ad),
				prov_sd, prov_sd_len);
	if (!err) {
		prov_fast_adv = false;
	}

	return err;
}
#endif
