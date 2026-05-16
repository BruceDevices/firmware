/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MESH_LOG_MODULE BLE_MESH_ADV_LOG


#include "adv.h"
#include "net.h"
#include "proxy.h"
#include "pb_gatt_srv.h"
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/nimble/host/include/host/ble_gap.h"

#if MYNEWT_VAL(BLE_MESH_ADV_EXT)
/* Convert from ms to 0.625ms units */
#define ADV_INT_FAST_MS    20
#define BT_ID_DEFAULT      0

static struct ble_gap_ext_adv_params adv_param = {
	.itvl_min = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
	.itvl_max = BT_MESH_ADV_SCAN_UNIT(ADV_INT_FAST_MS),
};

bool ext_adv_configured = false;

enum {
	/** Controller is currently advertising */
	ADV_FLAG_ACTIVE,
	/** Currently performing proxy advertising */
	ADV_FLAG_PROXY,
	/** The send-call has been scheduled. */
	ADV_FLAG_SCHEDULED,
	/** Custom adv params have been set, we need to update the parameters on
	 *  the next send.
	 */
	ADV_FLAG_UPDATE_PARAMS,

	/* Number of adv flags. */
	ADV_FLAGS_NUM
};

static struct {
	ATOMIC_DEFINE(flags, ADV_FLAGS_NUM);
	struct bt_le_ext_adv *instance;
	struct os_mbuf *buf;
	int64_t timestamp;
	struct k_work_delayable work;
} adv;


static void schedule_send(void)
{
	int64_t timestamp = adv.timestamp;
	int64_t delta;

	if (atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
		ble_gap_ext_adv_stop(BT_ID_DEFAULT);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
	}

	if (atomic_test_bit(adv.flags, ADV_FLAG_ACTIVE) ||
	atomic_test_and_set_bit(adv.flags, ADV_FLAG_SCHEDULED)) {
		return;
	}

	/* The controller will send the next advertisement immediately.
	 * Introduce a delay here to avoid sending the next mesh packet closer
	 * to the previous packet than what's permitted by the specification.
	 */
	delta = k_uptime_delta(&timestamp);
	k_work_reschedule(&adv.work, K_MSEC(ADV_INT_FAST_MS - delta));
}

static int
ble_mesh_ext_adv_event_handler(struct ble_gap_event *event, void *arg)
{
	int64_t duration;

	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		if (atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
			atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
			schedule_send();
		}
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		/* Calling k_uptime_delta on a timestamp moves it to the current time.
		 * This is essential here, as schedule_send() uses the end of the event
		 * as a reference to avoid sending the next advertisement too soon.
		 */
		duration = k_uptime_delta(&adv.timestamp);

		BT_DBG("Advertising stopped after %u ms", (uint32_t)duration);

		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);

		if (!atomic_test_and_clear_bit(adv.flags, ADV_FLAG_PROXY)) {
			net_buf_unref(adv.buf);
		}

		schedule_send();
		break;
	default:
		return 0;
	}
	return 0;
}

static int adv_start(const struct ble_gap_ext_adv_params *param,
		     uint32_t timeout,
		     const struct bt_data *ad, size_t ad_len,
		     const struct bt_data *sd, size_t sd_len)
{
	int err;
	struct os_mbuf *ad_data;
	struct os_mbuf *sd_data;

	ad_data = os_msys_get_pkthdr(BLE_HS_ADV_MAX_SZ, 0);
	assert(ad_data);
	sd_data = os_msys_get_pkthdr(BLE_HS_ADV_MAX_SZ, 0);
	assert(sd_data);
	if (!adv.instance) {
		BT_ERR("Mesh advertiser not enabled");
		err = -ENODEV;
		goto error;
	}

	if (atomic_test_and_set_bit(adv.flags, ADV_FLAG_ACTIVE)) {
		BT_ERR("Advertiser is busy");
		err = -EBUSY;
		goto error;
	}

	if (atomic_test_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS)) {
		err = ble_gap_ext_adv_configure(BT_ID_DEFAULT, param, NULL,
			       ble_mesh_ext_adv_event_handler, NULL);
		if (err) {
			BT_ERR("Failed updating adv params: %d", err);
			atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
			goto error;
		}

		atomic_set_bit_to(adv.flags, ADV_FLAG_UPDATE_PARAMS,
				  param != &adv_param);
	}

	assert(ad_data);
	err = os_mbuf_append(ad_data, ad, ad_len);
	if (err) {
		goto error;
	}

	err = ble_gap_ext_adv_set_data(BT_ID_DEFAULT, ad_data);
	if (err) {
		BT_ERR("Failed setting adv data: %d", err);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
		goto error;
	}

	err = os_mbuf_append(sd_data, sd, sd_len);
	if (err) {
		goto error;
	}
	err = ble_gap_ext_adv_rsp_set_data(BT_ID_DEFAULT, sd_data);
	if (err) {
		BT_ERR("Failed setting scan response data: %d", err);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
		goto error;
	}

	adv.timestamp = k_uptime_get();

	err = ble_gap_ext_adv_start(BT_ID_DEFAULT, timeout, 0);
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		atomic_clear_bit(adv.flags, ADV_FLAG_ACTIVE);
	}

error:
	if (ad_data) {
		os_mbuf_free_chain(ad_data);
	}

	if (sd_data) {
		os_mbuf_free_chain(sd_data);
	}
	return err;
}

static int buf_send(struct os_mbuf *buf)
{
	static const uint8_t bt_mesh_adv_type[] = {
		[BT_MESH_ADV_PROV]   = BLE_HS_ADV_TYPE_MESH_PROV,
		[BT_MESH_ADV_DATA]   = BLE_HS_ADV_TYPE_MESH_MESSAGE,
		[BT_MESH_ADV_BEACON] = BLE_HS_ADV_TYPE_MESH_BEACON,
		[BT_MESH_ADV_URI]    = BLE_HS_ADV_TYPE_URI,
	};

	struct bt_le_ext_adv_start_param start = {
		.num_events =
			BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1,
	};
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = MAX(ADV_INT_FAST_MS,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
	/* Upper boundary estimate: */
	duration = start.num_events * (adv_int + 10);

	BT_DBG("type %u len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->om_len, bt_hex(buf->om_data, buf->om_len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

	ad.type = bt_mesh_adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->om_len;
	ad.data = buf->om_data;

	/* Only update advertising parameters if they're different */
	if (adv_param.itvl_min != BT_MESH_ADV_SCAN_UNIT(adv_int)) {
		adv_param.itvl_min = BT_MESH_ADV_SCAN_UNIT(adv_int);
		adv_param.itvl_max = adv_param.itvl_min;
		atomic_set_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS);
	}

	err = adv_start(&adv_param, duration, &ad, 1, NULL, 0);
	if (!err) {
		adv.buf = net_buf_ref(buf);
	}

	bt_mesh_adv_send_start(duration, err, BT_MESH_ADV(buf));

	return err;
}

static void send_pending_adv(struct ble_npl_event *work)
{
	struct os_mbuf *buf;
	int err;

	atomic_clear_bit(adv.flags, ADV_FLAG_SCHEDULED);

	while ((buf = net_buf_get(&bt_mesh_adv_queue, K_NO_WAIT))) {
		/* busy == 0 means this was canceled */
		if (!BT_MESH_ADV(buf)->busy) {
			net_buf_unref(buf);
			continue;
		}

		BT_MESH_ADV(buf)->busy = 0U;
		err = buf_send(buf);

		net_buf_unref(buf);

		if (!err) {
			return; /* Wait for advertising to finish */
		}
	}

	if (!MYNEWT_VAL(BLE_MESH_GATT_SERVER)) {
		return;
	}

	/* No more pending buffers */
	if (bt_mesh_is_provisioned()) {
	    if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
	        err = bt_mesh_proxy_adv_start();
	        BT_DBG("Proxy Advertising");
	    }
	} else if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
	    err = bt_mesh_pb_gatt_adv_start();
	    BT_DBG("PB-GATT Advertising");
	}

	if (!err) {
		atomic_set_bit(adv.flags, ADV_FLAG_PROXY);
	}
}

void bt_mesh_adv_update(void)
{
	BT_DBG("");

	schedule_send();
}

void bt_mesh_adv_buf_ready(void)
{
	schedule_send();
}

void bt_mesh_adv_init(void)
{
    int rc;

    rc = os_mempool_init(&adv_buf_mempool, MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT),
                         BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE,
                         adv_buf_mem, "adv_buf_pool");
    assert(rc == 0);

    rc = os_mbuf_pool_init(&adv_os_mbuf_pool, &adv_buf_mempool,
                           BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE,
                           MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT));
    assert(rc == 0);

    ble_npl_eventq_init(&bt_mesh_adv_queue);

	k_work_init_delayable(&adv.work, send_pending_adv);
}

int bt_mesh_adv_enable(void)
{
    /* No need to initialize extended advertiser instance here */
	return 0;
}

int bt_mesh_adv_start(const struct ble_gap_adv_params *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len)
{
	static uint32_t adv_timeout;
	struct ble_gap_ext_adv_params params = {
		.itvl_min = param->itvl_min,
		.itvl_max = param->itvl_max
	};

	/* In NimBLE duration is in ms, not 10ms units */
	adv_timeout = (duration == BLE_HS_FOREVER) ? 0 : duration;

	BT_DBG("Start advertising %d ms", duration);

	atomic_set_bit(adv.flags, ADV_FLAG_UPDATE_PARAMS);

	return adv_start(&params, adv_timeout, ad, ad_len, sd, sd_len);
}
#endif
