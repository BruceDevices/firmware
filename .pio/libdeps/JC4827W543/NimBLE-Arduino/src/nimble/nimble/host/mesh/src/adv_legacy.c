/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#if MYNEWT_VAL(BLE_MESH)

#define MESH_LOG_MODULE BLE_MESH_ADV_LOG

#include "adv.h"
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"
#include "nimble/nimble/host/mesh/include/mesh/glue.h"
#include "pb_gatt_srv.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#if MYNEWT_VAL(BLE_MESH_ADV_LEGACY)
/* Convert from ms to 0.625ms units */
#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

#if (MYNEWT_VAL(BSP_NRF51) && !MYNEWT_VAL(BLE_CONTROLLER))
#define CONFIG_BT_CTLR_LOW_LAT 1
#else
#define CONFIG_BT_CTLR_LOW_LAT 0
#endif

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

static int32_t adv_int_min =  ADV_INT_DEFAULT_MS;

static int adv_initialized = false;
/* TinyCrypt PRNG consumes a lot of stack space, so we need to have
 * an increased call stack whenever it's used.
 */
#ifdef MYNEWT
OS_TASK_STACK_DEFINE(g_blemesh_stack, MYNEWT_VAL(BLE_MESH_ADV_STACK_SIZE));
struct os_task adv_task;
#endif

static int32_t adv_timeout;

static inline void adv_send(struct os_mbuf *buf)
{
	static const uint8_t adv_type[] = {
			[BT_MESH_ADV_PROV]   = BLE_HS_ADV_TYPE_MESH_PROV,
			[BT_MESH_ADV_DATA]   = BLE_HS_ADV_TYPE_MESH_MESSAGE,
			[BT_MESH_ADV_BEACON] = BLE_HS_ADV_TYPE_MESH_BEACON,
			[BT_MESH_ADV_URI]    = BLE_HS_ADV_TYPE_URI,
	};

	struct ble_gap_adv_params param = { 0 };
	uint16_t duration, adv_int;
	struct bt_data ad;
	int err;

	adv_int = MAX(adv_int_min,
		      BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
#if MYNEWT_VAL(BLE_CONTROLLER)
	duration = ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
				(adv_int + 10));
#else
	/* Zephyr Bluetooth Low Energy Controller for mesh stack uses
	 * pre-emptible continuous scanning, allowing advertising events to be
	 * transmitted without delay when advertising is enabled. No need to
	 * compensate with scan window duration.
	 * An advertising event could be delayed by upto one interval when
	 * advertising is stopped and started in quick succession, hence add
	 * advertising interval to the total advertising duration.
	 */
	duration = (adv_int +
		    ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
		     (adv_int + 10)));

	/* Zephyr Bluetooth Low Energy Controller built for nRF51x SoCs use
	 * CONFIG_BT_CTLR_LOW_LAT=y, and continuous scanning cannot be
	 * pre-empted, hence, scanning will block advertising events from
	 * being transmitted. Increase the advertising duration by the
	 * amount of scan window duration to compensate for the blocked
	 * advertising events.
	 */
	if (CONFIG_BT_CTLR_LOW_LAT) {
		duration += BT_MESH_SCAN_WINDOW_MS;
	}
#endif

	BT_DBG("type %u om_len %u: %s", BT_MESH_ADV(buf)->type,
	       buf->om_len, bt_hex(buf->om_data, buf->om_len));
	BT_DBG("count %u interval %ums duration %ums",
	       BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
	       duration);

	ad.type = adv_type[BT_MESH_ADV(buf)->type];
	ad.data_len = buf->om_len;
	ad.data = buf->om_data;

	param.itvl_min = ADV_SCAN_UNIT(adv_int);
	param.itvl_max = param.itvl_min;
	param.conn_mode = BLE_GAP_CONN_MODE_NON;

	int64_t time = k_uptime_get();

	err = bt_le_adv_start(&param, &ad, 1, NULL, 0);

	bt_mesh_adv_send_start(duration, err, BT_MESH_ADV(buf));
	if (err) {
		BT_ERR("Advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising started. Sleeping %u ms", duration);

	k_sleep(K_MSEC(duration));

	err = bt_le_adv_stop();
	if (err) {
		BT_ERR("Stopping advertising failed: err %d", err);
		return;
	}

	BT_DBG("Advertising stopped (%u ms)", (uint32_t) k_uptime_delta(&time));
}

void
mesh_adv_thread(void *args)
{
	static struct ble_npl_event *ev;
	struct os_mbuf *buf;

	BT_DBG("started");

	while (1) {
		if (MYNEWT_VAL(BLE_MESH_GATT_SERVER)) {
			ev = ble_npl_eventq_get(&bt_mesh_adv_queue, 0);
			while (!ev) {
				/* Adv timeout may be set by a call from proxy
				 * to bt_mesh_adv_start:
				 */
				adv_timeout = K_FOREVER;
				if (bt_mesh_is_provisioned()) {
					if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
						bt_mesh_proxy_adv_start();
						BT_DBG("Proxy Advertising up to %d ms", (int) adv_timeout);
					}
				} else if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
					bt_mesh_pb_gatt_adv_start();
					BT_DBG("PB-GATT Advertising up to %d ms", (int) adv_timeout);
				}

				ev = ble_npl_eventq_get(&bt_mesh_adv_queue, ble_npl_time_ms_to_ticks32(adv_timeout));
				bt_le_adv_stop();
			}
		} else {
			ev = ble_npl_eventq_get(&bt_mesh_adv_queue, BLE_NPL_TIME_FOREVER);
		}
		if (!ev || !ble_npl_event_get_arg(ev)) {
			continue;
		}

		buf = ble_npl_event_get_arg(ev);

		/* busy == 0 means this was canceled */
		if (BT_MESH_ADV(buf)->busy) {
			BT_MESH_ADV(buf)->busy = 0;
			adv_send(buf);
		}

		net_buf_unref(buf);

		/* os_sched(NULL); */
	}
}

void bt_mesh_adv_update(void)
{
	static struct ble_npl_event ev = { };

	BT_DBG("");

	ble_npl_eventq_put(&bt_mesh_adv_queue, &ev);
}

void bt_mesh_adv_buf_ready(void)
{
	/* Will be handled automatically */
}

void bt_mesh_adv_init(void)
{
	int rc;

	/* Advertising should only be initialized once. Calling
	 * os_task init the second time will result in an assert. */
	if (adv_initialized) {
		return;
	}

	rc = os_mempool_init(&adv_buf_mempool, MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT),
			     BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE,
			     adv_buf_mem, "adv_buf_pool");
	assert(rc == 0);

	rc = os_mbuf_pool_init(&adv_os_mbuf_pool, &adv_buf_mempool,
			       BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE,
			       MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT));
	assert(rc == 0);

	ble_npl_eventq_init(&bt_mesh_adv_queue);

#ifdef MYNEWT
	os_task_init(&adv_task, "mesh_adv", mesh_adv_thread, NULL,
	             MYNEWT_VAL(BLE_MESH_ADV_TASK_PRIO), OS_WAIT_FOREVER,
	             g_blemesh_stack, MYNEWT_VAL(BLE_MESH_ADV_STACK_SIZE));
#endif

	/* For BT5 controllers we can have fast advertising interval */
	if (ble_hs_hci_get_hci_version() >= BLE_HCI_VER_BCS_5_0) {
	    adv_int_min = ADV_INT_FAST_MS;
	}

	adv_initialized = true;
}

int bt_mesh_adv_enable(void)
{
	/* Dummy function - in legacy adv thread is started on init*/
	return 0;
}

int bt_mesh_adv_start(const struct ble_gap_adv_params *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len)
{
	adv_timeout = duration;
	return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}
#endif

#endif /* MYNEWT_VAL(BLE_MESH) */
