/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#define MESH_LOG_MODULE BLE_MESH_SETTINGS_LOG

#if MYNEWT_VAL(BLE_MESH_SETTINGS)

#include "mesh_priv.h"
#include "nimble/nimble/host/mesh/include/mesh/glue.h"
#include "subnet.h"
#include "app_keys.h"
#include "net.h"
#include "cdb_priv.h"
#include "rpl.h"
#include "crypto.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "pb_gatt_srv.h"
#include "proxy.h"
#include "settings.h"
#include "cfg.h"


#include "config/config.h"

static struct k_work_delayable pending_store;
static ATOMIC_DEFINE(pending_flags, BT_MESH_SETTINGS_FLAG_COUNT);

int settings_name_next(char *name, char **next)
{
	int rc = 0;

	if (next) {
		*next = NULL;
	}

	if (!name) {
		return 0;
	}

	/* name might come from flash directly, in flash the name would end
	 * with '=' or '\0' depending how storage is done. Flash reading is
	 * limited to what can be read
	 */
	while ((*name != '\0') && (*name != '=') &&
	       (*name != '/')) {
		rc++;
		name++;
	}

	if (*name == '/') {
		if (next) {
			*next = name + 1;
		}
		return rc;
	}

	return rc;
}

static int mesh_commit(void)
{
	if (!bt_mesh_subnet_next(NULL)) {
		/* Nothing to do since we're not yet provisioned */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		(void)bt_mesh_pb_gatt_disable();
	}

	bt_mesh_net_settings_commit();
	bt_mesh_model_settings_commit();

	atomic_set_bit(bt_mesh.flags, BT_MESH_VALID);

	bt_mesh_start();
	return 0;
}

/* Pending flags that use K_NO_WAIT as the storage timeout */
#define NO_WAIT_PENDING_BITS (BIT(BT_MESH_SETTINGS_NET_PENDING) |           \
			BIT(BT_MESH_SETTINGS_IV_PENDING)  |           \
			BIT(BT_MESH_SETTINGS_SEQ_PENDING) |           \
			BIT(BT_MESH_SETTINGS_CDB_PENDING))

/* Pending flags that use CONFIG_BT_MESH_STORE_TIMEOUT */
#define GENERIC_PENDING_BITS (BIT(BT_MESH_SETTINGS_NET_KEYS_PENDING) |      \
			BIT(BT_MESH_SETTINGS_APP_KEYS_PENDING) |      \
			BIT(BT_MESH_SETTINGS_HB_PUB_PENDING)   |      \
			BIT(BT_MESH_SETTINGS_CFG_PENDING)      |      \
			BIT(BT_MESH_SETTINGS_MOD_PENDING)      |      \
			BIT(BT_MESH_SETTINGS_VA_PENDING))

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
	int32_t timeout_ms, remaining_ms;

	atomic_set_bit(pending_flags, flag);

	if (atomic_get(pending_flags) & NO_WAIT_PENDING_BITS) {
		timeout_ms = 0;
	} else if (CONFIG_BT_MESH_RPL_STORE_TIMEOUT >= 0 &&
		   atomic_test_bit(pending_flags, BT_MESH_SETTINGS_RPL_PENDING) &&
		   !(atomic_get(pending_flags) & GENERIC_PENDING_BITS)) {
		timeout_ms = CONFIG_BT_MESH_RPL_STORE_TIMEOUT * MSEC_PER_SEC;
	} else {
		timeout_ms = CONFIG_BT_MESH_STORE_TIMEOUT * MSEC_PER_SEC;
	}

	remaining_ms = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&pending_store));
	BT_DBG("Waiting %u ms vs rem %u ms", timeout_ms, remaining_ms);
	/* If the new deadline is sooner, override any existing
	 * deadline; otherwise schedule without changing any existing
	 * deadline.
	 */
	if (timeout_ms < remaining_ms) {
		k_work_reschedule(&pending_store, K_MSEC(timeout_ms));
	} else {
		k_work_schedule(&pending_store, K_MSEC(timeout_ms));
	}
}

void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag)
{
	atomic_clear_bit(pending_flags, flag);
}

static void store_pending(struct ble_npl_event *work)
{
	BT_DBG("");
	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_RPL_PENDING)) {
		bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_NET_KEYS_PENDING)) {
		bt_mesh_subnet_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_APP_KEYS_PENDING)) {
		bt_mesh_app_key_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_NET_PENDING)) {
		bt_mesh_net_pending_net_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_IV_PENDING)) {
		bt_mesh_net_pending_iv_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_SEQ_PENDING)) {
		bt_mesh_net_pending_seq_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_HB_PUB_PENDING)) {
		bt_mesh_hb_pub_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_CFG_PENDING)) {
		bt_mesh_cfg_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_MOD_PENDING)) {
		bt_mesh_model_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_VA_PENDING)) {
		bt_mesh_va_pending_store();
	}

#if IS_ENABLED(CONFIG_BT_MESH_CDB)
	if (atomic_test_and_clear_bit(pending_flags,
				     BT_MESH_SETTINGS_CDB_PENDING)) {
		bt_mesh_cdb_pending_store();
	}
#endif
}

static struct conf_handler bt_mesh_settings_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = NULL,
	.ch_commit = mesh_commit,
	.ch_export = NULL,
};

void bt_mesh_settings_init(void)
{
	int rc;

	rc = conf_register(&bt_mesh_settings_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_settings conf");

	k_work_init_delayable(&pending_store, store_pending);
}

#endif /* MYNEWT_VAL(BLE_MESH_SETTINGS) */
