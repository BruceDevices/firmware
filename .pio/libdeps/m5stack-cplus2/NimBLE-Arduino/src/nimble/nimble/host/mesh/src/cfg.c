/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#include "nimble/nimble/host/mesh/include/mesh/mesh.h"
#include "mesh_priv.h"
#include "net.h"
#include "rpl.h"
#include "beacon.h"
#include "settings.h"
#include "heartbeat.h"
#include "friend.h"
#include "cfg.h"
#include "nimble/nimble/host/mesh/include/mesh/glue.h"

#define MESH_LOG_MODULE BLE_MESH_LOG
#include "nimble/porting/nimble/include/log/log.h"

/* Miscellaneous configuration server model states */
struct cfg_val {
	uint8_t net_transmit;
	uint8_t relay;
	uint8_t relay_retransmit;
	uint8_t beacon;
	uint8_t gatt_proxy;
	uint8_t frnd;
	uint8_t default_ttl;
};

void bt_mesh_beacon_set(bool beacon)
{
	if (atomic_test_bit(bt_mesh.flags, BT_MESH_BEACON) == beacon) {
		return;
	}

	atomic_set_bit_to(bt_mesh.flags, BT_MESH_BEACON, beacon);

	if (beacon) {
		bt_mesh_beacon_enable();
	} else {
		bt_mesh_beacon_disable();
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}
}

bool bt_mesh_beacon_enabled(void)
{
	return atomic_test_bit(bt_mesh.flags, BT_MESH_BEACON);
}

static int feature_set(int feature_flag, enum bt_mesh_feat_state state)
{
	if (state != BT_MESH_FEATURE_DISABLED &&
	    state != BT_MESH_FEATURE_ENABLED) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_mesh.flags, feature_flag) ==
	    (state == BT_MESH_FEATURE_ENABLED)) {
		return -EALREADY;
	}

	atomic_set_bit_to(bt_mesh.flags, feature_flag,
			  (state == BT_MESH_FEATURE_ENABLED));

	return 0;
}

static enum bt_mesh_feat_state feature_get(int feature_flag)
{
	return atomic_test_bit(bt_mesh.flags, feature_flag) ?
		       BT_MESH_FEATURE_ENABLED :
		       BT_MESH_FEATURE_DISABLED;
}

int bt_mesh_gatt_proxy_set(enum bt_mesh_feat_state gatt_proxy)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		return -ENOTSUP;
	}

	err = feature_set(BT_MESH_GATT_PROXY, gatt_proxy);
	if (err) {
		return err;
	}

	bt_mesh_hb_feature_changed(BT_MESH_FEAT_PROXY);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}

	return 0;
}

enum bt_mesh_feat_state bt_mesh_gatt_proxy_get(void)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		return BT_MESH_FEATURE_NOT_SUPPORTED;
	}

	return feature_get(BT_MESH_GATT_PROXY);
}

int bt_mesh_default_ttl_set(uint8_t default_ttl)
{
	if (default_ttl == 1 || default_ttl > BT_MESH_TTL_MAX) {
		return -EINVAL;
	}

	if (default_ttl == bt_mesh.default_ttl) {
		return 0;
	}

	bt_mesh.default_ttl = default_ttl;

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}

	return 0;
}

uint8_t bt_mesh_default_ttl_get(void)
{
	return bt_mesh.default_ttl;
}

int bt_mesh_friend_set(enum bt_mesh_feat_state friendship)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		return -ENOTSUP;
	}

	err = feature_set(BT_MESH_FRIEND, friendship);
	if (err) {
		return err;
	}

	bt_mesh_hb_feature_changed(BT_MESH_FEAT_FRIEND);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}

	if (friendship == BT_MESH_FEATURE_DISABLED) {
		bt_mesh_friends_clear();
	}

	return 0;
}

enum bt_mesh_feat_state bt_mesh_friend_get(void)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		return BT_MESH_FEATURE_NOT_SUPPORTED;
	}

	return feature_get(BT_MESH_FRIEND);
}

void bt_mesh_net_transmit_set(uint8_t xmit)
{
	if (bt_mesh.net_xmit == xmit) {
		return;
	}

	bt_mesh.net_xmit = xmit;

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}
}

uint8_t bt_mesh_net_transmit_get(void)
{
	return bt_mesh.net_xmit;
}

int bt_mesh_relay_set(enum bt_mesh_feat_state relay, uint8_t xmit)
{
	int err;

	if (!CONFIG_BT_MESH_RELAY) {
		return -ENOTSUP;
	}

	err = feature_set(BT_MESH_RELAY, relay);
	if (err == -EINVAL) {
		return err;
	}

	if (err == -EALREADY && bt_mesh.relay_xmit == xmit) {
		return -EALREADY;
	}

	bt_mesh.relay_xmit = xmit;
	bt_mesh_hb_feature_changed(BT_MESH_FEAT_RELAY);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_CFG_PENDING);
	}

	return 0;
}

enum bt_mesh_feat_state bt_mesh_relay_get(void)
{
	return feature_get(BT_MESH_RELAY);
}

uint8_t bt_mesh_relay_retransmit_get(void)
{
	if (!CONFIG_BT_MESH_RELAY) {
		return 0;
	}

	return bt_mesh.relay_xmit;
}

bool bt_mesh_fixed_group_match(uint16_t addr)
{
	/* Check for fixed group addresses */
	switch (addr) {
	case BT_MESH_ADDR_ALL_NODES:
		return true;
	case BT_MESH_ADDR_PROXIES:
		return (bt_mesh_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED);
	case BT_MESH_ADDR_FRIENDS:
		return (bt_mesh_friend_get() == BT_MESH_FEATURE_ENABLED);
	case BT_MESH_ADDR_RELAYS:
		return (bt_mesh_relay_get() == BT_MESH_FEATURE_ENABLED);
	default:
		return false;
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static int cfg_set(int argc, char **argv, char *val)
{
	struct cfg_val cfg;
	int len, err;

	BT_DBG("val %s", val ? val : "(null)");

	if (!val) {
		BT_DBG("Cleared configuration state");
		return 0;
	}

	len = sizeof(cfg);
	err = settings_bytes_from_str(val, &cfg, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(cfg)) {
		BT_ERR("Unexpected value length (%d != %zu)", len,
					       sizeof(cfg));
		return -EINVAL;
	}

	bt_mesh_net_transmit_set(cfg.net_transmit);
	bt_mesh_relay_set(cfg.relay, cfg.relay_retransmit);
	bt_mesh_beacon_set(cfg.beacon);
	bt_mesh_gatt_proxy_set(cfg.gatt_proxy);
	bt_mesh_friend_set(cfg.frnd);
	bt_mesh_default_ttl_set(cfg.default_ttl);

	BT_DBG("Restored configuration state");

	return 0;
}

static void clear_cfg(void)
{
	int err;

	err = settings_save_one("bt_mesh/Cfg", NULL);
	if (err) {
		BT_ERR("Failed to clear configuration");
	} else {
		BT_DBG("Cleared configuration");
	}
}

static void store_pending_cfg(void)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct cfg_val))];
	struct cfg_val val;
	char *str;
	int err;

	val.net_transmit = bt_mesh_net_transmit_get();
	val.relay = bt_mesh_relay_get();
	val.relay_retransmit = bt_mesh_relay_retransmit_get();
	val.beacon = bt_mesh_beacon_enabled();
	val.gatt_proxy = bt_mesh_gatt_proxy_get();
	val.frnd = bt_mesh_friend_get();
	val.default_ttl = bt_mesh_default_ttl_get();

	str = settings_str_from_bytes(&val, sizeof(val), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode configuration as value");
		return;
	}

	BT_DBG("Saving configuration as value %s", str);
	err = settings_save_one("bt_mesh/Cfg", str);
	if (err) {
		BT_ERR("Failed to store configuration");
	} else {
		BT_DBG("Stored configuration");
	}
}

void bt_mesh_cfg_pending_store(void)
{
	if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		store_pending_cfg();
	} else {
		clear_cfg();
	}
}

static struct conf_handler bt_mesh_cfg_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = cfg_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};
#endif

void bt_mesh_cfg_default_set(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int rc;

	rc = conf_register(&bt_mesh_cfg_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_settings conf");
#endif

	bt_mesh.default_ttl = CONFIG_BT_MESH_DEFAULT_TTL;
	bt_mesh.net_xmit =
		BT_MESH_TRANSMIT(CONFIG_BT_MESH_NETWORK_TRANSMIT_COUNT,
				 CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL);

#if defined(CONFIG_BT_MESH_RELAY)
	bt_mesh.relay_xmit =
		BT_MESH_TRANSMIT(CONFIG_BT_MESH_RELAY_RETRANSMIT_COUNT,
				 CONFIG_BT_MESH_RELAY_RETRANSMIT_INTERVAL);
#endif

	if (CONFIG_BT_MESH_RELAY_ENABLED) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_RELAY);
	}

	if (CONFIG_BT_MESH_BEACON_ENABLED) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_BEACON);
	}

	if (CONFIG_BT_MESH_GATT_PROXY_ENABLED) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_GATT_PROXY);
	}

	if (CONFIG_BT_MESH_FRIEND_ENABLED) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_FRIEND);
	}
}

#endif /* MYNEWT_VAL(BLE_MESH) */
