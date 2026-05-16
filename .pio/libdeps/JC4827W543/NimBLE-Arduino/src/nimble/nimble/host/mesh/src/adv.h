/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ADV_H__
#define __ADV_H__

/* Maximum advertising data payload for a single data type */
#include "nimble/nimble/host/mesh/include/mesh/mesh.h"

#define BT_MESH_ADV(om) (*(struct bt_mesh_adv **) OS_MBUF_USRHDR(om))

#define BT_MESH_ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)
#define BT_MESH_SCAN_INTERVAL_MS 30
#define BT_MESH_SCAN_WINDOW_MS   30

#define BT_MESH_ADV_DATA_SIZE 31

/* The user data is a pointer (4 bytes) to struct bt_mesh_adv */
#define BT_MESH_ADV_USER_DATA_SIZE 4

#define BT_MESH_MBUF_HEADER_SIZE (sizeof(struct os_mbuf_pkthdr) + \
                                    BT_MESH_ADV_USER_DATA_SIZE +\
				    sizeof(struct os_mbuf))

/* We declare it as extern here to share it between 'adv' and 'adv_legacy' */
extern struct os_mbuf_pool adv_os_mbuf_pool;
extern struct ble_npl_eventq bt_mesh_adv_queue;
extern struct os_mempool adv_buf_mempool;
extern os_membuf_t adv_buf_mem[];

enum bt_mesh_adv_type
{
	BT_MESH_ADV_PROV,
	BT_MESH_ADV_DATA,
	BT_MESH_ADV_BEACON,
	BT_MESH_ADV_URI,

	BT_MESH_ADV_TYPES,
};

typedef void (*bt_mesh_adv_func_t)(struct os_mbuf *buf, uint16_t duration,
				   int err, void *user_data);


static inline void adv_send_start(uint16_t duration, int err,
				  const struct bt_mesh_send_cb *cb,
				  	void *cb_data)
{
	if (cb && cb->start) {
		cb->start(duration, err, cb_data);
	}
}

struct bt_mesh_adv {
	/** Fragments associated with this buffer. */
	struct os_mbuf *frags;

	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	uint8_t      type:2,
		     started:1,
		     busy:1;

	uint8_t      xmit;

	uint8_t flags;

	int ref_cnt;
	struct ble_npl_event ev;
};

typedef struct bt_mesh_adv *(*bt_mesh_adv_alloc_t)(int id);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct os_mbuf *bt_mesh_adv_create(enum bt_mesh_adv_type type, uint8_t xmit,
				   int32_t timeout);

struct os_mbuf *bt_mesh_adv_create_from_pool(struct os_mbuf_pool *pool,
					     bt_mesh_adv_alloc_t get_id,
					     enum bt_mesh_adv_type type,
					     uint8_t xmit, int32_t timeout);

void bt_mesh_adv_send(struct os_mbuf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data);

void bt_mesh_adv_update(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);
int bt_mesh_adv_enable(void);

void bt_mesh_adv_buf_ready(void);

int bt_mesh_adv_start(const struct ble_gap_adv_params *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len);

static inline void bt_mesh_adv_send_start(uint16_t duration, int err,
					  struct bt_mesh_adv *adv)
{
	if (!adv->started) {
		adv->started = 1;

		if (adv->cb && adv->cb->start) {
			adv->cb->start(duration, err, adv->cb_data);
		}

		if (err) {
			adv->cb = NULL;
		}
	}
}

static inline void bt_mesh_adv_send_end(
	int err, struct bt_mesh_adv const *adv)
{
	if (adv->started && adv->cb && adv->cb->end) {
		adv->cb->end(err, adv->cb_data);
	}
}
int ble_adv_gap_mesh_cb(struct ble_gap_event *event, void *arg);
#endif
