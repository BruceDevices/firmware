/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_HEARTBEAT_LOG

#include "mesh_priv.h"
#include "net.h"
#include "rpl.h"
#include "access.h"
#include "lpn.h"
#include "settings.h"
#include "transport.h"
#include "heartbeat.h"
#include "foundation.h"
#include "nimble/nimble/host/mesh/include/mesh/glue.h"

/* Heartbeat Publication information for persistent storage. */
struct hb_pub_val {
	uint16_t dst;
	uint8_t  period;
	uint8_t  ttl;
	uint16_t feat;
	uint16_t net_idx:12,
		 indefinite:1;
};

struct bt_mesh_hb_cb hb_cb;

static struct bt_mesh_hb_pub pub;
static struct bt_mesh_hb_sub sub;
static struct k_work_delayable sub_timer;
static struct k_work_delayable pub_timer;

static int64_t sub_remaining(void)
{
	if (sub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return 0U;
	}

	uint32_t rem_ms = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&sub_timer));
	return rem_ms / MSEC_PER_SEC;
}

static void hb_publish_end_cb(int err, void *cb_data)
{
	if (pub.period && pub.count > 1) {
		k_work_reschedule(&pub_timer, K_SECONDS(pub.period));
	}

	if (pub.count != 0xffff) {
		pub.count--;
	}
}

static void notify_recv(uint8_t hops, uint16_t feat)
{
	sub.remaining = sub_remaining();

	if (hb_cb.recv != NULL) {
		hb_cb.recv(&sub, hops, feat);
	}
}

static void notify_sub_end(void)
{
	sub.remaining = 0;

	if (hb_cb.sub_end != NULL) {
		hb_cb.sub_end(&sub);
	}
}

static void sub_end(struct ble_npl_event *work)
{
	notify_sub_end();
}

static int heartbeat_send(const struct bt_mesh_send_cb *cb, void *cb_data)
{
	uint16_t feat = 0U;
	struct __packed {
		uint8_t init_ttl;
		uint16_t feat;
	} hb;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = pub.net_idx,
		.app_idx = BT_MESH_KEY_UNUSED,
		.addr = pub.dst,
		.send_ttl = pub.ttl,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh_subnet_get(pub.net_idx),
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = bt_mesh_net_transmit_get(),
	};

	/* Do nothing if heartbeat publication is not enabled or the subnet is
	 * removed.
	 */
	if (!tx.sub || pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return 0;
	}

	hb.init_ttl = pub.ttl;

	if (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED) {
		feat |= BT_MESH_FEAT_RELAY;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) {
		feat |= BT_MESH_FEAT_PROXY;
	}

	if (bt_mesh_friend_get() == BT_MESH_FRIEND_ENABLED) {
		feat |= BT_MESH_FEAT_FRIEND;
	}

	if (bt_mesh_lpn_established()) {
		feat |= BT_MESH_FEAT_LOW_POWER;
	}

	hb.feat = sys_cpu_to_be16(feat);

	BT_DBG("InitTTL %u feat 0x%04x", pub.ttl, feat);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_HEARTBEAT, &hb, sizeof(hb),
				cb, cb_data);
}

static void hb_publish_start_cb(uint16_t duration, int err, void *cb_data)
{
	if (err) {
		hb_publish_end_cb(err, cb_data);
	}
}

static void hb_publish(struct ble_npl_event *work)
{
	static const struct bt_mesh_send_cb publish_cb = {
		.start = hb_publish_start_cb,
		.end = hb_publish_end_cb,
	};
	struct bt_mesh_subnet *sub;
	int err;

	BT_DBG("hb_pub.count: %u", pub.count);

	/* Fast exit if disabled or expired */
	if (pub.period == 0U || pub.count == 0U) {
		return;
	}

	sub = bt_mesh_subnet_get(pub.net_idx);
	if (!sub) {
		BT_ERR("No matching subnet for idx 0x%02x", pub.net_idx);
		pub.dst = BT_MESH_ADDR_UNASSIGNED;
		return;
	}

	err = heartbeat_send(&publish_cb, NULL);
	if (err) {
		hb_publish_end_cb(err, NULL);
	}
}

int bt_mesh_hb_recv(struct bt_mesh_net_rx *rx, struct os_mbuf *buf)
{
	uint8_t init_ttl, hops;
	uint16_t feat;

	if (buf->om_len < 3) {
		BT_ERR("Too short heartbeat message");
		return -EINVAL;
	}

	init_ttl = (net_buf_simple_pull_u8(buf) & 0x7f);
	feat = net_buf_simple_pull_be16(buf);

	hops = (init_ttl - rx->ctx.recv_ttl + 1);

	if (rx->ctx.addr != sub.src || rx->ctx.recv_dst != sub.dst) {
		BT_DBG("No subscription for received heartbeat");
		return 0;
	}

	if (!k_work_delayable_is_pending(&sub_timer)) {
		BT_DBG("Heartbeat subscription inactive");
		return 0;
	}

	sub.min_hops = MIN(sub.min_hops, hops);
	sub.max_hops = MAX(sub.max_hops, hops);

	if (sub.count < 0xffff) {
		sub.count++;
	}

	BT_DBG("src 0x%04x TTL %u InitTTL %u (%u hop%s) feat 0x%04x",
	       rx->ctx.addr, rx->ctx.recv_ttl, init_ttl, hops,
	       (hops == 1U) ? "" : "s", feat);

	notify_recv(hops, feat);

	return 0;
}

static void pub_disable(void)
{
	BT_DBG("");

	pub.dst = BT_MESH_ADDR_UNASSIGNED;
	pub.count = 0U;
	pub.ttl = 0U;
	pub.period = 0U;

	k_work_cancel_delayable(&pub_timer);
}

uint8_t bt_mesh_hb_pub_set(struct bt_mesh_hb_pub *new_pub)
{
	if (!new_pub || new_pub->dst == BT_MESH_ADDR_UNASSIGNED) {
		pub_disable();

		if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
		    bt_mesh_is_provisioned()) {
			bt_mesh_settings_store_schedule(
				BT_MESH_SETTINGS_HB_PUB_PENDING);
		}

		return STATUS_SUCCESS;
	}

	if (!bt_mesh_subnet_get(new_pub->net_idx)) {
		BT_ERR("Unknown NetKey 0x%04x", new_pub->net_idx);
		return STATUS_INVALID_NETKEY;
	}

	new_pub->feat &= BT_MESH_FEAT_SUPPORTED;
	pub = *new_pub;

	if (!bt_mesh_is_provisioned()) {
		return STATUS_SUCCESS;
	}

	/* The first Heartbeat message shall be published as soon as possible
	 * after the Heartbeat Publication Period state has been configured for
	 * periodic publishing.
	 *
	 * If the new configuration disables publishing this flushes
	 * the work item.
	 */
	k_work_reschedule(&pub_timer, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_store_schedule(
			BT_MESH_SETTINGS_HB_PUB_PENDING);
	}

	return STATUS_SUCCESS;
}

void bt_mesh_hb_pub_get(struct bt_mesh_hb_pub *get)
{
	*get = pub;
}

uint8_t bt_mesh_hb_sub_set(uint16_t src, uint16_t dst, uint32_t period)
{
	if (src != BT_MESH_ADDR_UNASSIGNED && !BT_MESH_ADDR_IS_UNICAST(src)) {
		BT_WARN("Prohibited source address");
		return STATUS_INVALID_ADDRESS;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(dst) || BT_MESH_ADDR_IS_RFU(dst) ||
	    (BT_MESH_ADDR_IS_UNICAST(dst) && dst != bt_mesh_primary_addr())) {
		BT_WARN("Prohibited destination address");
		return STATUS_INVALID_ADDRESS;
	}

	if (period > (1U << 16)) {
		BT_WARN("Prohibited subscription period %" PRIu32 "s", period);
		return STATUS_CANNOT_SET;
	}

	/* Only an explicit address change to unassigned should trigger clearing
	 * of the values according to MESH/NODE/CFG/HBS/BV-02-C.
	 */
	if (src == BT_MESH_ADDR_UNASSIGNED || dst == BT_MESH_ADDR_UNASSIGNED) {
		sub.src = BT_MESH_ADDR_UNASSIGNED;
		sub.dst = BT_MESH_ADDR_UNASSIGNED;
		sub.min_hops = 0U;
		sub.max_hops = 0U;
		sub.count = 0U;
		sub.period = 0U;
	} else if (period) {
		sub.src = src;
		sub.dst = dst;
		sub.min_hops = BT_MESH_TTL_MAX;
		sub.max_hops = 0U;
		sub.count = 0U;
		sub.period = period;
	} else {
		/* Clearing the period should stop heartbeat subscription
		 * without clearing the parameters, so we can still read them.
		 */
		sub.period = 0U;
	}

	/* Start the timer, which notifies immediately if the new
	 * configuration disables the subscription.
	 */
	k_work_reschedule(&sub_timer, K_SECONDS(sub.period));

	return STATUS_SUCCESS;
}

void bt_mesh_hb_sub_reset_count(void)
{
	sub.count = 0;
}

void bt_mesh_hb_sub_get(struct bt_mesh_hb_sub *get)
{
	*get = sub;
	get->remaining = sub_remaining();
}

void bt_mesh_hb_feature_changed(uint16_t features)
{
	if (pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	if (!(pub.feat & features)) {
		return;
	}

	heartbeat_send(NULL, NULL);
}

void bt_mesh_hb_init(void)
{
	pub.net_idx = BT_MESH_KEY_UNUSED;
	k_work_init_delayable(&pub_timer, hb_publish);
	k_work_init_delayable(&sub_timer, sub_end);
}

void bt_mesh_hb_start(void)
{
	if (pub.count && pub.period) {
		BT_DBG("Starting heartbeat publication");
		k_work_reschedule(&pub_timer, K_NO_WAIT);
	}
}

void bt_mesh_hb_suspend(void)
{
	(void)k_work_cancel_delayable(&pub_timer);
}

void bt_mesh_hb_resume(void)
{
	if (pub.period && pub.count) {
		BT_DBG("Starting heartbeat publication");
		k_work_reschedule(&pub_timer, K_NO_WAIT);
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static int hb_pub_set(int argc, char **argv, char *val)
{
	struct bt_mesh_hb_pub pub;
	struct hb_pub_val hb_val;
	int len, err;
	BT_DBG("val %s", val ? val : "(null)");
	len = sizeof(hb_val);
	err = settings_bytes_from_str(val, &hb_val, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}
	if (len != sizeof(hb_val)) {
		BT_ERR("Unexpected value length (%d != %zu)", len,
		       sizeof(hb_val));
		return -EINVAL;
	}
	pub.dst = hb_val.dst;
	pub.period = bt_mesh_hb_pwr2(hb_val.period);
	pub.ttl = hb_val.ttl;
	pub.feat = hb_val.feat;
	pub.net_idx = hb_val.net_idx;
	if (hb_val.indefinite) {
		pub.count = 0xffff;
	} else {
		pub.count = 0;
	}
	(void) bt_mesh_hb_pub_set(&pub);

	BT_DBG("Restored heartbeat publication");

	return 0;
}

void bt_mesh_hb_pub_pending_store(void)
{
	struct bt_mesh_hb_pub pub;
	char buf[BT_SETTINGS_SIZE(sizeof(struct hb_pub_val))];
	struct hb_pub_val val;
	int err;
	char *str;

	bt_mesh_hb_pub_get(&pub);
	if (pub.dst == BT_MESH_ADDR_UNASSIGNED) {
		err = settings_save_one("bt_mesh/HBPub", NULL);
	} else {
		val.indefinite = (pub.count == 0xffff);
		val.dst = pub.dst;
		val.period = bt_mesh_hb_log(pub.period);
		val.ttl = pub.ttl;
		val.feat = pub.feat;
		val.net_idx = pub.net_idx;

		str = settings_str_from_bytes(&val, sizeof(val), buf, sizeof(buf));
		if (!str) {
			BT_ERR("Unable to encode configuration as value");
			return;
		}

		BT_DBG("Saving configuration as value %s", str);
		err = settings_save_one("bt_mesh/HBPub", str);
	}

	if (err) {
		BT_ERR("Failed to store Heartbeat Publication");
	} else {
		BT_DBG("Stored Heartbeat Publication");
	}
}

static struct conf_handler bt_mesh_hb_pub_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = hb_pub_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};
#endif

void bt_mesh_hb_pub_init(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int rc;

	rc = conf_register(&bt_mesh_hb_pub_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_hb_pub conf");
#endif
}

#endif /* MYNEWT_VAL(BLE_MESH) */
