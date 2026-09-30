/**
 * @file ble_controller.c
 *
 * @author Max McGee
 *
 * @brief Minimal BLE "serial" link over NimBLE, implementing the Nordic
 *        UART Service (NUS) GATT layout so off-the-shelf central-side
 *        tooling (bleak, nRF Connect, etc.) can talk to it directly:
 *
 *          Service UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 *          RX (write)  : 6E400002-B5A3-F393-E0A9-E50E24DCCA9E  (central -> us)
 *          TX (notify) : 6E400003-B5A3-F393-E0A9-E50E24DCCA9E  (us -> central)
 *
 * Only TX is actively used right now (pushing position updates). RX is
 * wired up and logged so we have a ready-made channel for commands later.
 *
 * REQUIRED sdkconfig (menuconfig):
 *   Component config -> Bluetooth -> Bluetooth  [enabled]
 *   Component config -> Bluetooth -> Bluetooth Host -> NimBLE - BLE only
 *   (make sure Bluedroid is NOT also selected as the host)
 *
 * REQUIRED component dependency (idf_component.yml / CMakeLists PRIV_REQUIRES):
 *   bt
 */

/*
 * Copyright (c) 2026 Saturn Sports
 * All rights reserved.
 *
 * This firmware is proprietary and confidential. Unauthorized use, modification,
 * or distribution is prohibited without written permission.
 */

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble_controller.h"
#include "buzzer_controller.h"

#define TAG "BLE_CTRL"

#define BLE_DEVICE_NAME "SaturnStrap"

/* --- Nordic UART Service UUIDs (128-bit, byte order per BLE_UUID128_INIT) --- */

static const ble_uuid128_t s_uart_svc_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                      0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t s_uart_rx_chr_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                      0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

static const ble_uuid128_t s_uart_tx_chr_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                      0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

static uint8_t  s_own_addr_type;
static uint16_t s_conn_handle   = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_tx_val_handle = 0;
static bool     s_notify_enabled = false;

/* Position payload sent on the TX characteristic. Packed to guarantee
 * a stable 8-byte little-endian wire layout regardless of struct padding. */
typedef struct __attribute__((packed)) {
    float   distance_mm;
    int32_t counts;
} ble_position_payload_t;

static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def s_gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_uart_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* Central -> device. Not used for anything yet; just logged. */
                .uuid       = &s_uart_rx_chr_uuid.u,
                .access_cb  = gatt_svr_chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                /* Device -> central. This is what we notify position on. */
                .uuid       = &s_uart_tx_chr_uuid.u,
                .access_cb  = gatt_svr_chr_access_cb,
                .val_handle = &s_tx_val_handle,
                .flags      = BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 } /* terminator */
        },
    },
    { 0 } /* terminator */
};

static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg){
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        ESP_LOGI(TAG, "RX write: %d bytes", ctxt->om->om_len);
        /* Nothing consumes this yet - reserved for future incoming commands. */
        return 0;
    }
    /* TX characteristic reads/notify subscription changes are handled
     * automatically by the NimBLE stack; nothing to do here. */
    return 0;
}

static void start_advertising(void);

static int gap_event_cb(struct ble_gap_event *event, void *arg){
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Central connected");
                s_conn_handle = event->connect.conn_handle;
            } else {
                ESP_LOGW(TAG, "Connect attempt failed (status=%d), re-advertising",
                          event->connect.status);
                start_advertising();
            }
            play_phrase();
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Central disconnected (reason=%d), re-advertising",
                      event->disconnect.reason);
            s_conn_handle    = BLE_HS_CONN_HANDLE_NONE;
            s_notify_enabled = false;
            start_advertising();
            play_phrase();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            start_advertising();
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            if (event->subscribe.attr_handle == s_tx_val_handle) {
                s_notify_enabled = event->subscribe.cur_notify;
                ESP_LOGI(TAG, "Central %s notifications",
                          s_notify_enabled ? "subscribed to" : "unsubscribed from");
            }
            return 0;

        default:
            return 0;
    }
}

static void start_advertising(void){
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields  fields      = {0};
    struct ble_hs_adv_fields  rsp_fields  = {0};

    /* Legacy advertising PDUs cap out at 31 bytes. flags + full name +
     * a full 128-bit service UUID doesn't fit in one packet, so the
     * UUID gets pushed into the scan response instead. */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.name             = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len         = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: rc=%d", rc);
        return;
    }

    rsp_fields.uuids128             = (ble_uuid128_t *)&s_uart_svc_uuid;
    rsp_fields.num_uuids128         = 1;
    rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields failed: rc=%d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                            &adv_params, gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed: rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising started as \"%s\"", BLE_DEVICE_NAME);
}

static void on_sync_cb(void){
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: rc=%d", rc);
        return;
    }
    start_advertising();
}

static void on_reset_cb(int reason){
    ESP_LOGW(TAG, "NimBLE host reset, reason=%d", reason);
}

static void nimble_host_task(void *param){
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run(); /* blocks until nimble_port_stop() is called */
    nimble_port_freertos_deinit();
}

void ble_controller_init(void){
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", err);
        return;
    }

    ble_hs_cfg.reset_cb = on_reset_cb;
    ble_hs_cfg.sync_cb  = on_sync_cb;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: rc=%d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: rc=%d", rc);
        return;
    }

    rc = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_svc_gap_device_name_set failed: rc=%d", rc);
        return;
    }

    /* Runs the NimBLE host as its own background FreeRTOS task. */
    nimble_port_freertos_init(nimble_host_task);

    ESP_LOGI(TAG, "BLE controller initialized");
}

bool ble_controller_is_connected(void){
    return s_conn_handle != BLE_HS_CONN_HANDLE_NONE;
}

void ble_controller_notify_position(float distance_mm, int32_t counts){
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || !s_notify_enabled) {
        return; /* nobody listening - drop it */
    }

    ble_position_payload_t payload = {
        .distance_mm = distance_mm,
        .counts      = counts,
    };

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&payload, sizeof(payload));
    if (om == NULL) {
        ESP_LOGW(TAG, "Failed to allocate mbuf for notify");
        return;
    }

    int rc = ble_gatts_notify_custom(s_conn_handle, s_tx_val_handle, om);
    if (rc != 0) {
        ESP_LOGW(TAG, "ble_gatts_notify_custom failed: rc=%d", rc);
    }
}