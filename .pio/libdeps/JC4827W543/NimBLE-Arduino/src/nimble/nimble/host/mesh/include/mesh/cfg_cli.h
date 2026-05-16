/** @file
 *  @brief Bluetooth Mesh Configuration Client Model APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MESH_CFG_CLI_H
#define __BT_MESH_CFG_CLI_H

/**
 * @brief Bluetooth Mesh
 * @defgroup bt_mesh_cfg_cli Bluetooth Mesh Configuration Client Model
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Mesh Configuration Client Model Context */
struct bt_mesh_cfg_cli {
	struct bt_mesh_model *model;
	struct bt_mesh_msg_ack_ctx ack_ctx;
};

extern const struct bt_mesh_model_op bt_mesh_cfg_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_cfg_cli_cb;

#define BT_MESH_MODEL_CFG_CLI(cli_data)                                        \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_CFG_CLI, bt_mesh_cfg_cli_op, NULL,   \
			 cli_data, &bt_mesh_cfg_cli_cb)

int bt_mesh_cfg_node_reset(uint16_t net_idx, uint16_t addr, bool *status);

int bt_mesh_cfg_comp_data_get(uint16_t net_idx, uint16_t addr, uint8_t page,
			      uint8_t *rsp, struct os_mbuf *comp);

int bt_mesh_cfg_beacon_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

int bt_mesh_cfg_krp_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			uint8_t *status, uint8_t *phase);

int bt_mesh_cfg_krp_set(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			uint8_t transition, uint8_t *status, uint8_t *phase);

int bt_mesh_cfg_beacon_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *status);

int bt_mesh_cfg_ttl_get(uint16_t net_idx, uint16_t addr, uint8_t *ttl);

int bt_mesh_cfg_ttl_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *ttl);

int bt_mesh_cfg_friend_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

int bt_mesh_cfg_friend_set(uint16_t net_idx, uint16_t addr, uint8_t val, uint8_t *status);

int bt_mesh_cfg_gatt_proxy_get(uint16_t net_idx, uint16_t addr, uint8_t *status);

int bt_mesh_cfg_gatt_proxy_set(uint16_t net_idx, uint16_t addr, uint8_t val,
			       uint8_t *status);

int bt_mesh_cfg_net_transmit_get(uint16_t net_idx, uint16_t addr,
			  uint8_t *transmit);

int bt_mesh_cfg_net_transmit_set(uint16_t net_idx, uint16_t addr,
		uint8_t val, uint8_t *transmit);

int bt_mesh_cfg_relay_get(uint16_t net_idx, uint16_t addr, uint8_t *status,
			  uint8_t *transmit);

int bt_mesh_cfg_relay_set(uint16_t net_idx, uint16_t addr, uint8_t new_relay,
			  uint8_t new_transmit, uint8_t *status, uint8_t *transmit);

int bt_mesh_cfg_net_key_add(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			    const uint8_t net_key[16], uint8_t *status);

int bt_mesh_cfg_net_key_get(uint16_t net_idx, uint16_t addr, uint16_t *keys,
			    size_t *key_cnt);

int bt_mesh_cfg_net_key_del(uint16_t net_idx, uint16_t addr,
			    uint16_t key_net_idx, uint8_t *status);

int bt_mesh_cfg_app_key_add(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			    uint16_t key_app_idx, const uint8_t app_key[16],
			    uint8_t *status);

int bt_mesh_cfg_app_key_get(uint16_t net_idx, uint16_t addr, uint16_t key_net_idx,
			    uint8_t *status, uint16_t *keys, size_t *key_cnt);

int bt_mesh_cfg_app_key_del(uint16_t net_idx, uint16_t addr,
		uint16_t key_net_idx, uint16_t key_app_idx, uint8_t *status);

int bt_mesh_cfg_mod_app_bind(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			     uint16_t mod_app_idx, uint16_t mod_id, uint8_t *status);

int bt_mesh_cfg_mod_app_unbind(uint16_t net_idx, uint16_t addr,
	uint16_t elem_addr, uint16_t mod_app_idx,
	uint16_t mod_id, uint8_t *status);

int bt_mesh_cfg_mod_app_bind_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				 uint16_t mod_app_idx, uint16_t mod_id, uint16_t cid,
				 uint8_t *status);

int bt_mesh_cfg_mod_app_unbind_vnd(uint16_t net_idx, uint16_t addr,
	uint16_t elem_addr, uint16_t mod_app_idx, uint16_t mod_id,
	uint16_t cid, uint8_t *status);

int bt_mesh_cfg_mod_app_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t mod_id, uint8_t *status, uint16_t *apps,
			    size_t *app_cnt);

int bt_mesh_cfg_mod_app_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint16_t cid, uint8_t *status,
				uint16_t *apps, size_t *app_cnt);

/** @def BT_MESH_PUB_PERIOD_100MS
 *
 *  @brief Helper macro to encode model publication period in units of 100ms
 *
 *  @param steps Number of 100ms steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_100MS(steps)  ((steps) & BIT_MASK(6))

/** @def BT_MESH_PUB_PERIOD_SEC
 *
 *  @brief Helper macro to encode model publication period in units of 1 second
 *
 *  @param steps Number of 1 second steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_SEC(steps)   (((steps) & BIT_MASK(6)) | (1 << 6))

/** @def BT_MESH_PUB_PERIOD_10SEC
 *
 *  @brief Helper macro to encode model publication period in units of 10
 *  seconds
 *
 *  @param steps Number of 10 second steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_10SEC(steps) (((steps) & BIT_MASK(6)) | (2 << 6))

/** @def BT_MESH_PUB_PERIOD_10MIN
 *
 *  @brief Helper macro to encode model publication period in units of 10
 *  minutes
 *
 *  @param steps Number of 10 minute steps.
 *
 *  @return Encoded value that can be assigned to bt_mesh_cfg_mod_pub.period
 */
#define BT_MESH_PUB_PERIOD_10MIN(steps) (((steps) & BIT_MASK(6)) | (3 << 6))

struct bt_mesh_cfg_mod_pub {
	uint16_t addr;
	const uint8_t *uuid;
	uint16_t  app_idx;
	bool   cred_flag;
	uint8_t   ttl;
	uint8_t   period;
	uint8_t   transmit;
};

int bt_mesh_cfg_mod_pub_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t mod_id, struct bt_mesh_cfg_mod_pub *pub,
			    uint8_t *status);

int bt_mesh_cfg_mod_pub_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint16_t cid,
				struct bt_mesh_cfg_mod_pub *pub, uint8_t *status);

int bt_mesh_cfg_mod_pub_set(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t mod_id, struct bt_mesh_cfg_mod_pub *pub,
			    uint8_t *status);

int bt_mesh_cfg_mod_pub_set_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint16_t cid,
				struct bt_mesh_cfg_mod_pub *pub, uint8_t *status);

int bt_mesh_cfg_mod_sub_add(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

int bt_mesh_cfg_mod_sub_add_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				 uint16_t sub_addr, uint16_t mod_id, uint16_t cid,
				 uint8_t *status);

int bt_mesh_cfg_mod_sub_del(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

int bt_mesh_cfg_mod_sub_del_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				 uint16_t sub_addr, uint16_t mod_id, uint16_t cid,
				 uint8_t *status);

int bt_mesh_cfg_mod_sub_overwrite(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				  uint16_t sub_addr, uint16_t mod_id, uint8_t *status);

int bt_mesh_cfg_mod_sub_overwrite_vnd(uint16_t net_idx, uint16_t addr,
				      uint16_t elem_addr, uint16_t sub_addr,
				      uint16_t mod_id, uint16_t cid, uint8_t *status);

int bt_mesh_cfg_mod_sub_va_add(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			       const uint8_t label[16], uint16_t mod_id,
			       uint16_t *virt_addr, uint8_t *status);

int bt_mesh_cfg_mod_sub_va_add_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				   const uint8_t label[16], uint16_t mod_id,
				   uint16_t cid, uint16_t *virt_addr, uint8_t *status);

int bt_mesh_cfg_mod_sub_va_del(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			       const uint8_t label[16], uint16_t mod_id,
			       uint16_t *virt_addr, uint8_t *status);

int bt_mesh_cfg_mod_sub_va_del_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				   const uint8_t label[16], uint16_t mod_id,
				   uint16_t cid, uint16_t *virt_addr, uint8_t *status);

int bt_mesh_cfg_mod_sub_va_overwrite(uint16_t net_idx, uint16_t addr,
				     uint16_t elem_addr, const uint8_t label[16],
				     uint16_t mod_id, uint16_t *virt_addr,
				     uint8_t *status);

int bt_mesh_cfg_mod_sub_va_overwrite_vnd(uint16_t net_idx, uint16_t addr,
					 uint16_t elem_addr, const uint8_t label[16],
					 uint16_t mod_id, uint16_t cid,
					 uint16_t *virt_addr, uint8_t *status);

int bt_mesh_cfg_mod_sub_get(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
			    uint16_t mod_id, uint8_t *status, uint16_t *subs,
			    size_t *sub_cnt);

int bt_mesh_cfg_mod_sub_get_vnd(uint16_t net_idx, uint16_t addr, uint16_t elem_addr,
				uint16_t mod_id, uint16_t cid, uint8_t *status,
				uint16_t *subs, size_t *sub_cnt);

struct bt_mesh_cfg_hb_sub {
	uint16_t src;
	uint16_t dst;
	uint8_t  period;
	uint8_t  count;
	uint8_t  min;
	uint8_t  max;
};

int bt_mesh_cfg_hb_sub_set(uint16_t net_idx, uint16_t addr,
			   struct bt_mesh_cfg_hb_sub *sub, uint8_t *status);

int bt_mesh_cfg_hb_sub_get(uint16_t net_idx, uint16_t addr,
			   struct bt_mesh_cfg_hb_sub *sub, uint8_t *status);

struct bt_mesh_cfg_hb_pub {
	uint16_t dst;
	uint8_t  count;
	uint8_t  period;
	uint8_t  ttl;
	uint16_t feat;
	uint16_t net_idx;
};

int bt_mesh_cfg_hb_pub_set(uint16_t net_idx, uint16_t addr,
			   const struct bt_mesh_cfg_hb_pub *pub, uint8_t *status);

int bt_mesh_cfg_hb_pub_get(uint16_t net_idx, uint16_t addr,
			   struct bt_mesh_cfg_hb_pub *pub, uint8_t *status);

int bt_mesh_cfg_mod_sub_del_all(uint16_t net_idx, uint16_t addr,
				uint16_t elem_addr, uint16_t mod_id,
				uint8_t *status);

int bt_mesh_cfg_mod_sub_del_all_vnd(uint16_t net_idx, uint16_t addr,
				    uint16_t elem_addr, uint16_t mod_id,
				    uint16_t cid, uint8_t *status);

int bt_mesh_cfg_net_key_update(uint16_t net_idx, uint16_t addr,
			       uint16_t key_net_idx, const uint8_t net_key[16],
			       uint8_t *status);

int bt_mesh_cfg_app_key_update(uint16_t net_idx, uint16_t addr,
			       uint16_t key_net_idx, uint16_t key_app_idx,
			       const uint8_t app_key[16], uint8_t *status);

int bt_mesh_cfg_node_identity_set(uint16_t net_idx, uint16_t addr,
				  uint16_t key_net_idx, uint8_t new_identity,
				  uint8_t *status, uint8_t *identity);

int bt_mesh_cfg_node_identity_get(uint16_t net_idx, uint16_t addr,
				  uint16_t key_net_idx, uint8_t *status,
				  uint8_t *identity);

int bt_mesh_cfg_lpn_timeout_get(uint16_t net_idx, uint16_t addr,
				uint16_t unicast_addr, int32_t *polltimeout);

int32_t bt_mesh_cfg_cli_timeout_get(void);
void bt_mesh_cfg_cli_timeout_set(int32_t timeout);

struct bt_mesh_comp_p0 {
	/** Company ID */
	uint16_t cid;
	/** Product ID */
	uint16_t pid;
	/** Version ID */
	uint16_t vid;
	/** Replay protection list size */
	uint16_t crpl;
	/** Supported features, see @ref BT_MESH_FEAT_SUPPORTED. */
	uint16_t feat;

	struct os_mbuf *_buf;
};

struct bt_mesh_comp_p0_elem {
	/** Element location */
	uint16_t loc;
	/** The number of SIG models in this element */
	size_t nsig;
	/** The number of vendor models in this element */
	size_t nvnd;

	uint8_t *_buf;
};

int bt_mesh_comp_p0_get(struct bt_mesh_comp_p0 *comp,
	struct os_mbuf *buf);

struct bt_mesh_comp_p0_elem *bt_mesh_comp_p0_elem_pull(const struct bt_mesh_comp_p0 *comp,
	struct bt_mesh_comp_p0_elem *elem);

uint16_t bt_mesh_comp_p0_elem_mod(struct bt_mesh_comp_p0_elem *elem, int idx);

struct bt_mesh_mod_id_vnd bt_mesh_comp_p0_elem_mod_vnd(struct bt_mesh_comp_p0_elem *elem, int idx);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* __BT_MESH_CFG_CLI_H */
