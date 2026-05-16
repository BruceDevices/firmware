/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_ACCESS_LOG

#include <errno.h>
#include <stdlib.h>
#include <nimble/porting/nimble/include/os/os_mbuf.h>

#include "nimble/nimble/host/mesh/include/mesh/mesh.h"

#include "mesh_priv.h"
#include "adv.h"
#include "net.h"
#include "lpn.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "settings.h"
#if MYNEWT_VAL(BLE_MESH_SHELL_MODELS)
#include "nimble/nimble/host/mesh/include/mesh/model_cli.h"
#endif

/* bt_mesh_model.flags */
enum {
	BT_MESH_MOD_BIND_PENDING = BIT(0),
	BT_MESH_MOD_SUB_PENDING = BIT(1),
	BT_MESH_MOD_PUB_PENDING = BIT(2),
	BT_MESH_MOD_EXTENDED = BIT(3),
};

/* Model publication information for persistent storage. */
struct mod_pub_val {
	uint16_t addr;
	uint16_t key;
	uint8_t  ttl;
	uint8_t  retransmit;
	uint8_t  period;
	uint8_t  period_div:4,
		 cred:1;
};

static const struct bt_mesh_comp *dev_comp;
static uint16_t dev_primary_addr;
static void (*msg_cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx, struct os_mbuf *buf);

void bt_mesh_model_foreach(void (*func)(struct bt_mesh_model *mod,
					struct bt_mesh_elem *elem,
					bool vnd, bool primary,
					void *user_data),
			   void *user_data)
{
	int i, j;

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		for (j = 0; j < elem->model_count; j++) {
			struct bt_mesh_model *model = &elem->models[j];

			func(model, elem, false, i == 0, user_data);
		}

		for (j = 0; j < elem->vnd_model_count; j++) {
			struct bt_mesh_model *model = &elem->vnd_models[j];

			func(model, elem, true, i == 0, user_data);
		}
	}
}

int32_t bt_mesh_model_pub_period_get(struct bt_mesh_model *mod)
{
	int period;

	if (!mod->pub) {
		return 0;
	}

	switch (mod->pub->period >> 6) {
	case 0x00:
		/* 1 step is 100 ms */
		period = K_MSEC((mod->pub->period & BIT_MASK(6)) * 100);
		break;
	case 0x01:
		/* 1 step is 1 second */
		period = K_SECONDS(mod->pub->period & BIT_MASK(6));
		break;
	case 0x02:
		/* 1 step is 10 seconds */
		period = K_SECONDS((mod->pub->period & BIT_MASK(6)) * 10);
		break;
	case 0x03:
		/* 1 step is 10 minutes */
		period = K_MINUTES((mod->pub->period & BIT_MASK(6)) * 10);
		break;
	default:
		CODE_UNREACHABLE;
	}

	if (mod->pub->fast_period) {
		return period >> mod->pub->period_div;
	} else {
		return period;
	}
}

static int32_t next_period(struct bt_mesh_model *mod)
{
	struct bt_mesh_model_pub *pub = mod->pub;
	uint32_t elapsed, period;

	period = bt_mesh_model_pub_period_get(mod);
	if (!period) {
		return 0;
	}

	elapsed = k_uptime_get_32() - pub->period_start;

	BT_DBG("Publishing took %ums", (unsigned) elapsed);

	if (elapsed > period) {
		BT_WARN("Publication sending took longer than the period");
		/* Return smallest positive number since 0 means disabled */
		return K_MSEC(1);
	}

	return period - elapsed;
}

static void publish_sent(int err, void *user_data)
{
	struct bt_mesh_model *mod = user_data;
	int32_t delay;

	BT_DBG("err %d", err);

	if (mod->pub->count) {
		delay = BT_MESH_PUB_TRANSMIT_INT(mod->pub->retransmit);
	} else {
		delay = next_period(mod);
	}

	if (delay) {
		BT_DBG("Publishing next time in %dms", (int) delay);
		k_work_schedule(&mod->pub->timer, delay);
	}
}

static void publish_start(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_model *mod = user_data;
	struct bt_mesh_model_pub *pub = mod->pub;

	if (err) {
		BT_ERR("Failed to publish: err %d", err);
		publish_sent(err, user_data);
		return;
	}

	/* Initialize the timestamp for the beginning of a new period */
	if (pub->count == BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit)) {
		pub->period_start = k_uptime_get_32();
	}
}

static const struct bt_mesh_send_cb pub_sent_cb = {
	.start = publish_start,
	.end = publish_sent,
};

static int publish_transmit(struct bt_mesh_model *mod)
{
	struct os_mbuf *sdu = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX);
	struct bt_mesh_model_pub *pub = mod->pub;
	struct bt_mesh_msg_ctx ctx = {
		.addr = pub->addr,
		.send_ttl = pub->ttl,
		.app_idx = pub->key,
	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = bt_mesh_model_elem(mod)->addr,
		.friend_cred = pub->cred,
	};
	int err;

	net_buf_simple_init(sdu, 0);
	net_buf_simple_add_mem(sdu, pub->msg->om_data, pub->msg->om_len);

	err = bt_mesh_trans_send(&tx, sdu, &pub_sent_cb, mod);

	os_mbuf_free_chain(sdu);
	return err;
}

static int pub_period_start(struct bt_mesh_model_pub *pub)
{
	int err;

	pub->count = BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit);

	if (!pub->update) {
		return 0;
	}

	err = pub->update(pub->mod);
	if (err) {
		/* Skip this publish attempt. */
		BT_DBG("Update failed, skipping publish (err: %d)", err);
		pub->count = 0;
		pub->period_start = k_uptime_get_32();
		publish_sent(err, pub->mod);
		return err;
	}

	return 0;
}

static void mod_publish(struct ble_npl_event *work)
{
	struct bt_mesh_model_pub *pub = ble_npl_event_get_arg(work);
	int err;

	if (pub->addr == BT_MESH_ADDR_UNASSIGNED ||
	atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		/* Publication is no longer active, but the cancellation of the
		 * delayed work failed. Abandon recurring timer.
		 */
		return;
	}

	BT_DBG("");

	if (pub->count) {
		pub->count--;
	} else {
		/* First publication in this period */
		err = pub_period_start(pub);
		if (err) {
			return;
		}
	}

	err = publish_transmit(pub->mod);
	if (err) {
		BT_ERR("Failed to publish (err %d)", err);
		if (pub->count == BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit)) {
			pub->period_start = k_uptime_get_32();
		}

		publish_sent(err, pub->mod);
	}
}

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return &dev_comp->elem[mod->elem_idx];
}

struct bt_mesh_model *bt_mesh_model_get(bool vnd, uint8_t elem_idx, uint8_t mod_idx)
{
	struct bt_mesh_elem *elem;

	if (elem_idx >= dev_comp->elem_count) {
		BT_ERR("Invalid element index %u", elem_idx);
		return NULL;
	}

	elem = &dev_comp->elem[elem_idx];

	if (vnd) {
		if (mod_idx >= elem->vnd_model_count) {
			BT_ERR("Invalid vendor model index %u", mod_idx);
			return NULL;
		}

		return &elem->vnd_models[mod_idx];
	} else {
		if (mod_idx >= elem->model_count) {
			BT_ERR("Invalid SIG model index %u", mod_idx);
			return NULL;
		}

		return &elem->models[mod_idx];
	}
}

#if defined(CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE)
static int bt_mesh_vnd_mod_msg_cid_check(struct bt_mesh_model *mod)
{
	uint16_t cid;
	const struct bt_mesh_model_op *op;

	for (op = mod->op; op->func; op++) {
		cid = (uint16_t)(op->opcode & 0xffff);

		if (cid == mod->vnd.company) {
			continue;
		}

		BT_ERR("Invalid vendor model(company:0x%04x"
		       " id:0x%04x) message opcode 0x%08" PRIx32,
		       mod->vnd.company, mod->vnd.id, op->opcode);

		return -EINVAL;
	}

	return 0;
}
#endif

static void mod_init(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		     bool vnd, bool primary, void *user_data)
{
	int i;
	int *err = user_data;

	if (*err) {
		return;
	}

	if (mod->pub) {
		mod->pub->mod = mod;
		k_work_init_delayable(&mod->pub->timer, mod_publish);
		k_work_add_arg_delayable(&mod->pub->timer, mod->pub);
	}

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	mod->elem_idx = elem - dev_comp->elem;
	if (vnd) {
		mod->mod_idx = mod - elem->vnd_models;

		if (CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE) {
			*err = bt_mesh_vnd_mod_msg_cid_check(mod);
			if (*err) {
				return;
			}
		}
	} else {
		mod->mod_idx = mod - elem->models;
	}

	if (mod->cb && mod->cb->init) {
		*err = mod->cb->init(mod);
	}
}

int bt_mesh_comp_register(const struct bt_mesh_comp *comp)
{
	int err;

	/* There must be at least one element */
	if (!comp || !comp->elem_count) {
		return -EINVAL;
	}

	dev_comp = comp;

	err = 0;
	bt_mesh_model_foreach(mod_init, &err);

	return err;
}

void bt_mesh_comp_provision(uint16_t addr)
{
	int i;

	dev_primary_addr = addr;

	BT_DBG("addr 0x%04x elem_count %zu", addr, dev_comp->elem_count);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[i];

		elem->addr = addr++;

		BT_DBG("addr 0x%04x mod_count %u vnd_mod_count %u",
		       elem->addr, elem->model_count, elem->vnd_model_count);
	}
}

void bt_mesh_comp_unprovision(void)
{
	BT_DBG("");

	dev_primary_addr = BT_MESH_ADDR_UNASSIGNED;
}

uint16_t bt_mesh_primary_addr(void)
{
	return dev_primary_addr;
}

static uint16_t *model_group_get(struct bt_mesh_model *mod, uint16_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] == addr) {
			return &mod->groups[i];
		}
	}

	return NULL;
}

struct find_group_visitor_ctx {
	uint16_t *entry;
	struct bt_mesh_model *mod;
	uint16_t addr;
};

static enum bt_mesh_walk find_group_mod_visitor(struct bt_mesh_model *mod, void *user_data)
{
	struct find_group_visitor_ctx *ctx = user_data;

	if (mod->elem_idx != ctx->mod->elem_idx) {
		return BT_MESH_WALK_CONTINUE;
	}

	ctx->entry = model_group_get(mod, ctx->addr);
	if (ctx->entry) {
		ctx->mod = mod;
		return BT_MESH_WALK_STOP;
	}

	return BT_MESH_WALK_CONTINUE;
}

uint16_t *bt_mesh_model_find_group(struct bt_mesh_model **mod, uint16_t addr)
{
	struct find_group_visitor_ctx ctx = {
		.mod = *mod,
		.entry = NULL,
		.addr = addr,
	};

	bt_mesh_model_extensions_walk(*mod, find_group_mod_visitor, &ctx);

	*mod = ctx.mod;
	return ctx.entry;
}

static struct bt_mesh_model *bt_mesh_elem_find_group(struct bt_mesh_elem *elem,
						     uint16_t group_addr)
{
	struct bt_mesh_model *model;
	uint16_t *match;
	int i;

	for (i = 0; i < elem->model_count; i++) {
		model = &elem->models[i];

		match = model_group_get(model, group_addr);
		if (match) {
			return model;
		}
	}

	for (i = 0; i < elem->vnd_model_count; i++) {
		model = &elem->vnd_models[i];

		match = model_group_get(model, group_addr);
		if (match) {
			return model;
		}
	}

	return NULL;
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	uint16_t index;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return NULL;
	}

	index = addr - dev_comp->elem[0].addr;
	if (index >= dev_comp->elem_count) {
		return NULL;
	}

	return &dev_comp->elem[index];
}

bool bt_mesh_has_addr(uint16_t addr)
{
	uint16_t index;

	if (BT_MESH_ADDR_IS_UNICAST(addr)) {
		return bt_mesh_elem_find(addr) != NULL;
	}

	if (MYNEWT_VAL(BLE_MESH_ACCESS_LAYER_MSG) && msg_cb) {
		return true;
	}

	for (index = 0; index < dev_comp->elem_count; index++) {
		struct bt_mesh_elem *elem = &dev_comp->elem[index];

		if (bt_mesh_elem_find_group(elem, addr)) {
			return true;
		}
	}

	return false;
}

#if MYNEWT_VAL(BLE_MESH_ACCESS_LAYER_MSG)
void bt_mesh_msg_cb_set(void (*cb)(uint32_t opcode, struct bt_mesh_msg_ctx *ctx,
			struct os_mbuf *buf))
{
	msg_cb = cb;
}
#endif

int bt_mesh_msg_send(struct bt_mesh_msg_ctx *ctx, struct os_mbuf *buf, uint16_t src_addr,
		     const struct bt_mesh_send_cb *cb, void *cb_data)
{
	struct bt_mesh_net_tx tx = {
		.ctx = ctx,
		.src = src_addr,
	};

	BT_DBG("net_idx 0x%04x app_idx 0x%04x dst 0x%04x", tx.ctx->net_idx,
	       tx.ctx->app_idx, tx.ctx->addr);
	BT_DBG("len %u: %s", buf->om_len, bt_hex(buf->om_data, buf->om_len));

	if (!bt_mesh_is_provisioned()) {
		BT_ERR("Local node is not yet provisioned");
		return -EAGAIN;
	}

	return bt_mesh_trans_send(&tx, buf, cb, cb_data);
}

uint8_t bt_mesh_elem_count(void)
{
	return dev_comp->elem_count;
}

bool bt_mesh_model_has_key(struct bt_mesh_model *mod, uint16_t key)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		if (mod->keys[i] == key ||
		    (mod->keys[i] == BT_MESH_KEY_DEV_ANY &&
		     BT_MESH_IS_DEV_KEY(key))) {
			return true;
		}
	}

	return false;
}

static bool model_has_dst(struct bt_mesh_model *mod, uint16_t dst)
{
	if (BT_MESH_ADDR_IS_UNICAST(dst)) {
		return (dev_comp->elem[mod->elem_idx].addr == dst);
	} else if (BT_MESH_ADDR_IS_GROUP(dst) || BT_MESH_ADDR_IS_VIRTUAL(dst)) {
		return !!bt_mesh_model_find_group(&mod, dst);
	}

	/* If a message with a fixed group address is sent to the access layer,
	 * the lower layers have already confirmed that we are subscribing to
	 * it. All models on the primary element should receive the message.
	 */
	return mod->elem_idx == 0;
}

static const struct bt_mesh_model_op *find_op(struct bt_mesh_elem *elem,
					      uint32_t opcode, struct bt_mesh_model **model)
{
	uint8_t i;
	uint8_t count;
	/* This value shall not be used in shipping end products. */
	uint32_t cid = UINT32_MAX;
	struct bt_mesh_model *models;

	/* SIG models cannot contain 3-byte (vendor) OpCodes, and
	 * vendor models cannot contain SIG (1- or 2-byte) OpCodes, so
	 * we only need to do the lookup in one of the model lists.
	 */
	if (BT_MESH_MODEL_OP_LEN(opcode) < 3) {
		models = elem->models;
		count = elem->model_count;
	} else {
		models = elem->vnd_models;
		count = elem->vnd_model_count;

		cid = (uint16_t)(opcode & 0xffff);
	}

	for (i = 0U; i < count; i++) {
		const struct bt_mesh_model_op *op;

		if (CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE &&
		cid != UINT32_MAX &&
		cid != models[i].vnd.company) {
			continue;
		}
		*model = &models[i];

		for (op = (*model)->op; op->func; op++) {
			if (op->opcode == opcode) {
				return op;
			}
		}
	}

	*model = NULL;
	return NULL;
}

static int get_opcode(struct os_mbuf *buf, uint32_t *opcode)
{
	switch (buf->om_data[0] >> 6) {
	case 0x00:
	case 0x01:
		if (buf->om_data[0] == 0x7f) {
			BT_ERR("Ignoring RFU OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_u8(buf);
		return 0;
	case 0x02:
		if (buf->om_len < 2) {
			BT_ERR("Too short payload for 2-octet OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_be16(buf);
		return 0;
	case 0x03:
		if (buf->om_len < 3) {
			BT_ERR("Too short payload for 3-octet OpCode");
			return -EINVAL;
		}

		*opcode = net_buf_simple_pull_u8(buf) << 16;
		/* Using LE for the CID since the model layer is defined as
		 * little-endian in the mesh spec and using BT_MESH_MODEL_OP_3
		 * will declare the opcode in this way.
		 */
		*opcode |= net_buf_simple_pull_le16(buf);
		return 0;
	}

	CODE_UNREACHABLE;
}

void bt_mesh_model_recv(struct bt_mesh_net_rx *rx, struct os_mbuf *buf)
{
	struct bt_mesh_model *model;
	const struct bt_mesh_model_op *op;
	uint32_t opcode;
	int i;

	BT_DBG("app_idx 0x%04x src 0x%04x dst 0x%04x", rx->ctx.app_idx,
	       rx->ctx.addr, rx->ctx.recv_dst);
	BT_DBG("len %u: %s", buf->om_len, bt_hex(buf->om_data, buf->om_len));

	if (get_opcode(buf, &opcode) < 0) {
		BT_WARN("Unable to decode OpCode");
		return;
	}

	BT_DBG("OpCode 0x%08x", (unsigned) opcode);

	for (i = 0; i < dev_comp->elem_count; i++) {
		struct net_buf_simple_state state;

		op = find_op(&dev_comp->elem[i], opcode, &model);

		if (!op) {
			BT_DBG("No OpCode 0x%08x for elem %d", opcode, i);
			continue;
		}

		if (!bt_mesh_model_has_key(model, rx->ctx.app_idx)) {
				continue;
			}

		if (!model_has_dst(model, rx->ctx.recv_dst)) {
			continue;
		}

		if ((op->len >= 0) && (buf->om_len < (size_t)op->len)) {
			BT_ERR("Too short message for OpCode 0x%08" PRIx32, opcode);
			continue;
		} else if ((op->len < 0) && (buf->om_len != (size_t)(-op->len))) {
			BT_ERR("Invalid message size for OpCode 0x%08" PRIx32,
			       opcode);
			continue;
		}

		/* The callback will likely parse the buffer, so
		 * store the parsing state in case multiple models
		 * receive the message.
		 */
		net_buf_simple_save(buf, &state);
		(void)op->func(model, &rx->ctx, buf);
		net_buf_simple_restore(buf, &state);
	}

	if (MYNEWT_VAL(BLE_MESH_ACCESS_LAYER_MSG) && msg_cb) {
		msg_cb(opcode, &rx->ctx, buf);
	}
}

int bt_mesh_model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct os_mbuf *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data)
{
	if (!bt_mesh_model_has_key(model, ctx->app_idx)) {
		BT_ERR("Model not bound to AppKey 0x%04x", ctx->app_idx);
		return -EINVAL;
	}

	return bt_mesh_msg_send(ctx, msg, bt_mesh_model_elem(model)->addr, cb, cb_data);
}

int bt_mesh_model_publish(struct bt_mesh_model *model)
{
	struct bt_mesh_model_pub *pub = model->pub;

	if (!pub) {
		return -ENOTSUP;
	}

	BT_DBG("");

	if (pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return -EADDRNOTAVAIL;
	}

	if (!pub->msg || !pub->msg->om_len) {
		BT_ERR("No publication message");
		return -EINVAL;
	}

	if (pub->msg->om_len + BT_MESH_MIC_SHORT > BT_MESH_TX_SDU_MAX) {
		BT_ERR("Message does not fit maximum SDU size");
		return -EMSGSIZE;
	}

	if (pub->count) {
		BT_WARN("Clearing publish retransmit timer");
	}

	/* Account for initial transmission */
	pub->count = BT_MESH_PUB_TRANSMIT_COUNT(pub->retransmit) + 1;

	BT_DBG("Publish Retransmit Count %u Interval %ums", pub->count,
	       BT_MESH_PUB_TRANSMIT_INT(pub->retransmit));

	k_work_reschedule(&pub->timer, K_NO_WAIT);

	return 0;
}

struct bt_mesh_model *bt_mesh_model_find_vnd(const struct bt_mesh_elem *elem,
					     uint16_t company, uint16_t id)
{
	uint8_t i;

	for (i = 0; i < elem->vnd_model_count; i++) {
		if (elem->vnd_models[i].vnd.company == company &&
		    elem->vnd_models[i].vnd.id == id) {
			return &elem->vnd_models[i];
		}
	}

	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
					 uint16_t id)
{
	uint8_t i;

	for (i = 0; i < elem->model_count; i++) {
		if (elem->models[i].id == id) {
			return &elem->models[i];
		}
	}

	return NULL;
}

const struct bt_mesh_comp *bt_mesh_comp_get(void)
{
	return dev_comp;
}

void bt_mesh_model_extensions_walk(struct bt_mesh_model *model,
				   enum bt_mesh_walk (*cb)(struct bt_mesh_model *mod,
				   			   void *user_data),
				   void *user_data)
{
#ifndef CONFIG_BT_MESH_MODEL_EXTENSIONS
	(void)cb(model, user_data);
	return;
#else
	struct bt_mesh_model *it;

	if (cb(model, user_data) == BT_MESH_WALK_STOP || !model->next) {
		return;
	}
	/* List is circular. Step through all models until we reach the start: */
	for (it = model->next; it != model; it = it->next) {
		if (cb(it, user_data) == BT_MESH_WALK_STOP) {
			return;
		}
	}
#endif
}

#if MYNEWT_VAL(BLE_MESH_MODEL_EXTENSIONS)
int bt_mesh_model_extend(struct bt_mesh_model *extending_mod, struct bt_mesh_model *base_mod)
{
	struct bt_mesh_model *a = extending_mod;
	struct bt_mesh_model *b = base_mod;
	struct bt_mesh_model *a_next = a->next;
	struct bt_mesh_model *b_next = b->next;
	struct bt_mesh_model *it;

	base_mod->flags |= BT_MESH_MOD_EXTENDED;

	if (a == b) {
		return 0;
	}

	/* Check if a's list contains b */
	for (it = a; (it != NULL) && (it->next != a); it = it->next) {
		if (it == b) {
			return 0;
		}
	}

	/* Merge lists */
	if (a_next) {
		b->next = a_next;
	} else {
		b->next = a;
	}

	if (b_next) {
		a->next = b_next;
	} else {
		a->next = b;
	}

	return 0;
}
#endif

bool bt_mesh_model_is_extended(struct bt_mesh_model *model)
{
	return model->flags & BT_MESH_MOD_EXTENDED;
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static int mod_set_bind(struct bt_mesh_model *mod, char *val)
{
	int len, err, i;

	/* Start with empty array regardless of cleared or set value */
	for (i = 0; i < ARRAY_SIZE(mod->keys); i++) {
		mod->keys[i] = BT_MESH_KEY_UNUSED;
	}

	if (!val) {
		BT_DBG("Cleared bindings for model");
		return 0;
	}

	len = sizeof(mod->keys);
	err = settings_bytes_from_str(val, mod->keys, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return -EINVAL;
	}

	BT_DBG("Decoded %u bound keys for model", len / sizeof(mod->keys[0]));
	return 0;
}

static int mod_set_sub(struct bt_mesh_model *mod, char *val)
{
	int len, err;

	/* Start with empty array regardless of cleared or set value */
	memset(mod->groups, 0, sizeof(mod->groups));

	if (!val) {
		BT_DBG("Cleared subscriptions for model");
		return 0;
	}

	len = sizeof(mod->groups);
	err = settings_bytes_from_str(val, mod->groups, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return -EINVAL;
	}

	BT_DBG("Decoded %u subscribed group addresses for model",
	       len / sizeof(mod->groups[0]));
	return 0;
}

static int mod_set_pub(struct bt_mesh_model *mod, char *val)
{
	struct mod_pub_val pub;
	int len, err;

	if (!mod->pub) {
		BT_WARN("Model has no publication context!");
		return -EINVAL;
	}

	if (!val) {
		mod->pub->addr = BT_MESH_ADDR_UNASSIGNED;
		mod->pub->key = 0;
		mod->pub->cred = 0;
		mod->pub->ttl = 0;
		mod->pub->period = 0;
		mod->pub->retransmit = 0;
		mod->pub->period_div = pub.period_div;
		mod->pub->count = 0;

		BT_DBG("Cleared publication for model");
		return 0;
	}

	len = sizeof(pub);
	err = settings_bytes_from_str(val, &pub, &len);
	if (err) {
		BT_ERR("Failed to decode value %s (err %d)", val, err);
		return -EINVAL;
	}

	if (len != sizeof(pub)) {
		BT_ERR("Invalid length for model publication");
		return -EINVAL;
	}

	mod->pub->addr = pub.addr;
	mod->pub->key = pub.key;
	mod->pub->cred = pub.cred;
	mod->pub->ttl = pub.ttl;
	mod->pub->period = pub.period;
	mod->pub->retransmit = pub.retransmit;
	mod->pub->period_div = pub.period_div;
	mod->pub->count = 0;

	BT_DBG("Restored model publication, dst 0x%04x app_idx 0x%03x",
			       pub.addr, pub.key);

	return 0;
}

static int mod_data_set(struct bt_mesh_model *mod,
			char *name, char *len_rd)
{
	char *next;

	settings_name_next(name, &next);

	if (mod->cb && mod->cb->settings_set) {
		return mod->cb->settings_set(mod, next, len_rd);
	}

	return 0;
}

static int mod_set(bool vnd, int argc, char **argv, char *val)
{
	struct bt_mesh_model *mod;
	uint8_t elem_idx, mod_idx;
	uint16_t mod_key;

	if (argc < 2) {
			BT_ERR("Too small argc (%d)", argc);
			return -ENOENT;
		}

		mod_key = strtol(argv[0], NULL, 16);
	elem_idx = mod_key >> 8;
	mod_idx = mod_key;

	BT_DBG("Decoded mod_key 0x%04x as elem_idx %u mod_idx %u",
			       mod_key, elem_idx, mod_idx);

	mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);
	if (!mod) {
		BT_ERR("Failed to get model for elem_idx %u mod_idx %u",
					       elem_idx, mod_idx);
		return -ENOENT;
	}

	if (!strcmp(argv[1], "bind")) {
		return mod_set_bind(mod, val);
	}

	if (!strcmp(argv[1], "sub")) {
		return mod_set_sub(mod, val);
	}

	if (!strcmp(argv[1], "pub")) {
		return mod_set_pub(mod, val);
	}

	if (!strcmp(argv[1], "data")) {
			return mod_data_set(mod, argv[1], val);
	}

	BT_WARN("Unknown module key %s", argv[1]);
	return -ENOENT;
}

static int sig_mod_set(int argc, char **argv, char *val)
{
	return mod_set(false, argc, argv, val);
}

static int vnd_mod_set(int argc, char **argv, char *val)
{
	return mod_set(true, argc, argv, val);
}

static void encode_mod_path(struct bt_mesh_model *mod, bool vnd,
			    const char *key, char *path, size_t path_len)
{
	uint16_t mod_key = (((uint16_t)mod->elem_idx << 8) | mod->mod_idx);

	if (vnd) {
		snprintk(path, path_len, "bt_mesh/v/%x/%s", mod_key, key);
	} else {
		snprintk(path, path_len, "bt_mesh/s/%x/%s", mod_key, key);
	}
}

static void store_pending_mod_bind(struct bt_mesh_model *mod, bool vnd)
{
	uint16_t keys[CONFIG_BT_MESH_MODEL_KEY_COUNT];
	char buf[BT_SETTINGS_SIZE(sizeof(keys))];
	char path[20];
	int i, count, err;
	char *val;

	for (i = 0, count = 0; i < ARRAY_SIZE(mod->keys); i++) {
		if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
			keys[count++] = mod->keys[i];
			BT_DBG("model key 0x%04x", mod->keys[i]);
		}
	}

	if (count) {
		val = settings_str_from_bytes(keys, count * sizeof(keys[0]),
					      buf, sizeof(buf));
		if (!val) {
			BT_ERR("Unable to encode model bindings as value");
			return;
		}
	} else {
		val = NULL;
	}

	encode_mod_path(mod, vnd, "bind", path, sizeof(path));

	BT_DBG("Saving %s as %s", path, val ? val : "(null)");
	err = settings_save_one(path, val);
	if (err) {
		BT_ERR("Failed to store bind");
	} else {
		BT_DBG("Stored bind");
	}
}

static void store_pending_mod_sub(struct bt_mesh_model *mod, bool vnd)
{
	uint16_t groups[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
	char buf[BT_SETTINGS_SIZE(sizeof(groups))];
	char path[20];
	int i, count, err;
	char *val;

	for (i = 0, count = 0; i < CONFIG_BT_MESH_MODEL_GROUP_COUNT; i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
				groups[count++] = mod->groups[i];
			}
	}

	if (count) {
		val = settings_str_from_bytes(groups, count * sizeof(groups[0]),
				      buf, sizeof(buf));
		if (!val) {
			BT_ERR("Unable to encode model subscription as value");
			return;
		}
	} else {
		val = NULL;
	}

	encode_mod_path(mod, vnd, "sub", path, sizeof(path));

	BT_DBG("Saving %s as %s", path, val ? val : "(null)");
	err = settings_save_one(path, val);
	if (err) {
		BT_ERR("Failed to store sub");
	} else {
		BT_DBG("Stored sub");
	}
}

static void store_pending_mod_pub(struct bt_mesh_model *mod, bool vnd)
{
	char buf[BT_SETTINGS_SIZE(sizeof(struct mod_pub_val))];
	struct mod_pub_val pub;
	char path[20];
	char *val;
	int err;

	if (!mod->pub || mod->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		val = NULL;
	} else {
		pub.addr = mod->pub->addr;
		pub.key = mod->pub->key;
		pub.ttl = mod->pub->ttl;
		pub.retransmit = mod->pub->retransmit;
		pub.period = mod->pub->period;
		pub.period_div = mod->pub->period_div;
		pub.cred = mod->pub->cred;

		val = settings_str_from_bytes(&pub, sizeof(pub), buf, sizeof(buf));
		if (!val) {
			BT_ERR("Unable to encode model publication as value");
			return;
		}
	}

	encode_mod_path(mod, vnd, "pub", path, sizeof(path));

	BT_DBG("Saving %s as %s", path, val ? val : "(null)");
	err = settings_save_one(path, val);
	if (err) {
		BT_ERR("Failed to store pub");
	} else {
		BT_DBG("Stored pub");
	}
}

static void store_pending_mod(struct bt_mesh_model *mod,
			      struct bt_mesh_elem *elem, bool vnd,
			      bool primary, void *user_data)
{
	if (!mod->flags) {
		return;
	}

	if (mod->flags & BT_MESH_MOD_BIND_PENDING) {
		mod->flags &= ~BT_MESH_MOD_BIND_PENDING;
		store_pending_mod_bind(mod, vnd);
	}

	if (mod->flags & BT_MESH_MOD_SUB_PENDING) {
		mod->flags &= ~BT_MESH_MOD_SUB_PENDING;
		store_pending_mod_sub(mod, vnd);
	}

	if (mod->flags & BT_MESH_MOD_PUB_PENDING) {
		mod->flags &= ~BT_MESH_MOD_PUB_PENDING;
		store_pending_mod_pub(mod, vnd);
	}
}

void bt_mesh_model_pending_store(void)
{
	bt_mesh_model_foreach(store_pending_mod, NULL);
}

void bt_mesh_model_bind_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_BIND_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

void bt_mesh_model_sub_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_SUB_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

void bt_mesh_model_pub_store(struct bt_mesh_model *mod)
{
	mod->flags |= BT_MESH_MOD_PUB_PENDING;
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
}

int bt_mesh_model_data_store(struct bt_mesh_model *mod, bool vnd,
			     const char *name, const void *data,
			     size_t data_len)
{
	char path[30];
	char buf[BT_SETTINGS_SIZE(sizeof(struct mod_pub_val))];
	char *val;
	int err;

	encode_mod_path(mod, vnd, "data", path, sizeof(path));
	if (name) {
		strcat(path, "/");
		strncat(path, name, SETTINGS_MAX_DIR_DEPTH);
	}

	if (data_len) {
		val = settings_str_from_bytes(data, data_len, buf, sizeof(buf));
		if (!val) {
			BT_ERR("Unable to encode model publication as value");
			return -EINVAL;
		}
		err = settings_save_one(path, val);
	} else {
		err = settings_save_one(path, NULL);
	}

	if (err) {
		BT_ERR("Failed to store %s value", path);
	} else {
		BT_DBG("Stored %s value", path);
	}
	return err;
}
#endif

static void commit_mod(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
		       bool vnd, bool primary, void *user_data)
{
	if (mod->pub && mod->pub->update &&
	    	mod->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int32_t ms = bt_mesh_model_pub_period_get(mod);

		if (ms > 0) {
			BT_DBG("Starting publish timer (period %u ms)", ms);
			k_work_schedule(&mod->pub->timer, K_MSEC(ms));
		}
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(mod->groups); i++) {
		if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			bt_mesh_lpn_group_add(mod->groups[i]);
		}
	}
}

void bt_mesh_model_settings_commit(void)
{
	bt_mesh_model_foreach(commit_mod, NULL);
}

#if MYNEWT_VAL(BLE_MESH_SETTINGS)
static struct conf_handler bt_mesh_sig_mod_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = sig_mod_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};

static struct conf_handler bt_mesh_vnd_mod_conf_handler = {
	.ch_name = "bt_mesh",
	.ch_get = NULL,
	.ch_set = vnd_mod_set,
	.ch_commit = NULL,
	.ch_export = NULL,
};
#endif

void bt_mesh_access_init(void)
{
    #if MYNEWT_VAL(BLE_MESH_SETTINGS)
	int rc;

	rc = conf_register(&bt_mesh_sig_mod_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_access conf");
	rc = conf_register(&bt_mesh_vnd_mod_conf_handler);

	SYSINIT_PANIC_ASSERT_MSG(rc == 0,
				 "Failed to register bt_mesh_access conf");
    #endif
}

#endif /* MYNEWT_VAL(BLE_MESH) */
