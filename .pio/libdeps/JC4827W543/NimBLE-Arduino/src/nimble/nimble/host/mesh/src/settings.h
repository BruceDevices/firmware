/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Pending storage actions. */
enum bt_mesh_settings_flag {
	BT_MESH_SETTINGS_RPL_PENDING,
	BT_MESH_SETTINGS_NET_KEYS_PENDING,
	BT_MESH_SETTINGS_APP_KEYS_PENDING,
	BT_MESH_SETTINGS_NET_PENDING,
	BT_MESH_SETTINGS_IV_PENDING,
	BT_MESH_SETTINGS_SEQ_PENDING,
	BT_MESH_SETTINGS_HB_PUB_PENDING,
	BT_MESH_SETTINGS_CFG_PENDING,
	BT_MESH_SETTINGS_MOD_PENDING,
	BT_MESH_SETTINGS_VA_PENDING,
	BT_MESH_SETTINGS_CDB_PENDING,

	BT_MESH_SETTINGS_FLAG_COUNT,
};

void bt_mesh_settings_init(void);
int settings_name_next(char *name, char **next);
void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag);
void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag);
