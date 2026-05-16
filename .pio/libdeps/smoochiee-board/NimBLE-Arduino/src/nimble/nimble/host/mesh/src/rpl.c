/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_RPL_LOG

#include "nimble/porting/nimble/include/log/log.h"
#include <stdlib.h>

#include "mesh_priv.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "settings.h"

/* Replay Protection List information for persistent storage. */
struct rpl_val {
	uint32_t seq:24,
	old_iv:1;
};

static struct bt_mesh_rpl replay_list[MYNEWT_VAL(BLE_MESH_CRPL)];
static ATOMIC_DEFINE(store, MYNEWT_VAL(BLE_MESH_CRPL));

static inline int rpl_idx(const struct bt_mesh_rpl *rpl)
{
	return rpl - &replay_list[0];
}

static void clear_rpl(struct bt_mesh_rpl *rpl)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int err;
	char path[18];

	if (!rpl->src) {
		return;
	}

	snprintk(path, sizeof(path), "bt_mesh/RPL/%x", rpl->src);
	err = settings_save_one(path, NULL);
	if (err) {
		BT_ERR("Failed to clear RPL");
	} else {
		BT_DBG("Cleared RPL");
	}

	(void)memset(rpl, 0, sizeof(*rpl));
	atomic_clear_bit(store, rpl_idx(rpl));
#endif
}

static void schedule_rpl_store(struct bt_mesh_rpl *entry, bool force)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	atomic_set_bit(store, rpl_idx(entry));
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	if (force
#ifdef CONFIG_BT_MESH_RPL_STORE_TIMEOUT
	|| CONFIG_BT_MESH_RPL_STORE_TIMEOUT >= 0
#endif
	    ) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	}
#endif
}

static void schedule_rpl_clear(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
#endif
}

void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
		struct bt_mesh_net_rx *rx)
{
	/* If this is the first message on the new IV index, we should reset it
	 * to zero to avoid invalid combinations of IV index and seg.
	 */
	if (rpl->old_iv && !rx->old_iv) {
		rpl->seg = 0;
	}

	rpl->src = rx->ctx.addr;
	rpl->seq = rx->seq;
	rpl->old_iv = rx->old_iv;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		schedule_rpl_store(rpl, false);
	}
}

/* Check the Replay Protection List for a replay attempt. If non-NULL match
 * parameter is given the RPL slot is returned but it is not immediately
 * updated (needed for segmented messages), whereas if a NULL match is given
 * the RPL is immediately updated (used for unsegmented messages).
 */
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx,
		struct bt_mesh_rpl **match)
{
	int i;

	/* Don't bother checking messages from ourselves */
	if (rx->net_if == BT_MESH_NET_IF_LOCAL) {
		return false;
	}

	/* The RPL is used only for the local node */
	if (!rx->local_match) {
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		/* Empty slot */
		if (!rpl->src) {
			if (match) {
				*match = rpl;
			} else {
				bt_mesh_rpl_update(rpl, rx);
			}

			return false;
		}

		/* Existing slot for given address */
		if (rpl->src == rx->ctx.addr) {
			if (rx->old_iv && !rpl->old_iv) {
				return true;
			}

			if ((!rx->old_iv && rpl->old_iv) ||
			    rpl->seq < rx->seq) {
				if (match) {
					*match = rpl;
				} else {
					bt_mesh_rpl_update(rpl, rx);
				}

				return false;
			} else {
				return true;
			}
		}
	}

	BT_ERR("RPL is full!");
	return true;
}

void bt_mesh_rpl_clear(void)
{
	BT_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		schedule_rpl_clear();
	} else {
		(void)memset(replay_list, 0, sizeof(replay_list));
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static struct bt_mesh_rpl *bt_mesh_rpl_find(uint16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (replay_list[i].src == src) {
			return &replay_list[i];
		}
	}

	return NULL;
}

static struct bt_mesh_rpl *bt_mesh_rpl_alloc(uint16_t src)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (!replay_list[i].src) {
			replay_list[i].src = src;
			return &replay_list[i];
		}
	}

	return NULL;
}
#endif

void bt_mesh_rpl_reset(void)
{
	int i;

	/* Discard "old old" IV Index entries from RPL and flag
	 * any other ones (which are valid) as old.
	 */
	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		struct bt_mesh_rpl *rpl = &replay_list[i];

		if (rpl->src) {
			if (rpl->old_iv) {
				if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
					clear_rpl(rpl);
				} else {
					(void)memset(rpl, 0, sizeof(*rpl));
				}
			} else {
				rpl->old_iv = true;
				if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
					schedule_rpl_store(rpl, true);
				}
			}
		}
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static int rpl_set(int argc, char **argv, char *val)
{
	struct bt_mesh_rpl *entry;
	struct rpl_val rpl;
	int len, err;
	uint16_t src;

	if (argc < 1) {
		BT_ERR("Invalid argc (%d)", argc);
		return -ENOENT;
	}

	BT_DBG("argv[0] %s val %s", argv[0], val ? val : "(null)");

	src = strtol(argv[0], NULL, 16);
	entry = bt_mesh_rpl_find(src);

	if (!val) {
		if (entry) {
			memset(entry, 0, sizeof(*entry));
		} else {
			BT_WARN("Unable to find RPL entry for 0x%04x", src);
		}

		return 0;
	}

	if (!entry) {
		entry = bt_mesh_rpl_alloc(src);
		if (!entry) {
			BT_ERR("Unable to allocate RPL entry for 0x%04x", src);
			return -ENOMEM;
		}
	}

	len = sizeof(rpl);
	err = settings_bytes_from_str(val, &rpl, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return err;
	}

	if (len != sizeof(rpl)) {
		BT_ERR("Unexpected value length (%d != %zu)", len, sizeof(rpl));
		return -EINVAL;
	}

	entry->seq = rpl.seq;
	entry->old_iv = rpl.old_iv;

	BT_DBG("RPL entry for 0x%04x: Seq 0x%06x old_iv %u", entry->src,
	       (unsigned) entry->seq, entry->old_iv);
	return 0;
}
#endif

static void store_rpl(struct bt_mesh_rpl *entry)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	char buf[BT_SETTINGS_SIZE(sizeof(struct rpl_val))];
	struct rpl_val rpl;
	char path[18];
	char *str;
	int err;

	if (!entry->src) {
		return;
	}

	BT_DBG("src 0x%04x seq 0x%06x old_iv %u", entry->src,
	       (unsigned) entry->seq, entry->old_iv);

	rpl.seq = entry->seq;
	rpl.old_iv = entry->old_iv;

	str = settings_str_from_bytes(&rpl, sizeof(rpl), buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode RPL as value");
		return;
	}

	snprintk(path, sizeof(path), "bt_mesh/RPL/%x", entry->src);

	BT_DBG("Saving RPL %s as value %s", path, str);
	err = settings_save_one(path, str);
	if (err) {
		BT_ERR("Failed to store RPL");
	} else {
		BT_DBG("Stored RPL");
	}
#endif
}

static void store_pending_rpl(struct bt_mesh_rpl *rpl)
{
	BT_DBG("");

	if (atomic_test_and_clear_bit(store, rpl_idx(rpl))) {
		store_rpl(rpl);
	}
}

void bt_mesh_rpl_pending_store(uint16_t addr)
{
	int i;

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) ||
	(!BT_MESH_ADDR_IS_UNICAST(addr) &&
	addr != BT_MESH_ADDR_ALL_NODES)) {
		return;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_PENDING);
	}

	for (i = 0; i < ARRAY_SIZE(replay_list); i++) {
		if (addr != BT_MESH_ADDR_ALL_NODES &&
		addr != replay_list[i].src) {
			continue;
		}

		if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
			store_pending_rpl(&replay_list[i]);
		} else {
			clear_rpl(&replay_list[i]);
		}

		if (addr != BT_MESH_ADDR_ALL_NODES) {
			break;
		}
	}
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static struct conf_handler bt_mesh_rpl_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = rpl_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};
#endif

void bt_mesh_rpl_init(void)
{
#if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int rc;

	rc = conf_register(&bt_mesh_rpl_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_rpl conf");
#endif
}

#endif /* MYNEWT_VAL(BLE_MESH) */
