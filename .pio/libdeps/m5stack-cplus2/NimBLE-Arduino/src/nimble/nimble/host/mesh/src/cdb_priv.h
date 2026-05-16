/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* ommitted `bt_mesh_cdb_node_store` declaration - every header
 * includes mesh/mesh.h, which already has it
 * void bt_mesh_cdb_node_store(const struct bt_mesh_cdb_node *node);
 */
void bt_mesh_cdb_pending_store(void);
void bt_mesh_cdb_init(void);
