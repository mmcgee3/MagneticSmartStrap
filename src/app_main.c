/**
 * @file app_main.c
 * 
 * @author Max McGee
 *
 * @brief Main program file
 * 
 * TO DO
 */

/*
 * Copyright (c) 2026 Saturn Sports
 * All rights reserved.
 *
 * This firmware is proprietary and confidential. Unauthorized use, modification,
 * or distribution is prohibited without written permission.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_config.h" 
#include "ble_controller.h"
#include "buzzer_controller.h"
#include "hall_sensor_controller.h"

#define TASK_DELAY_MS 250

#define TAG "APP_MAIN"


void app_main(void){
    buzzer_init();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Application starting up!");
    buzzer_set_state(true);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    buzzer_set_state(false);

    esp_err_t nvs_err = nvs_flash_init();

    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);


    // INIT BLE
    hall_sensor_init();
    ble_controller_init();

    hall_sensor_reset();

    int last_counts = hall_sensor_get_counts();

    // Main Loop
    while(1) {
        int counts = hall_sensor_get_counts();

        // Only push a BLE update when the position has actually moved.
        if (counts != last_counts) {
            float distance = hall_sensor_get_distance_mm();

            ESP_LOGI("app", "%.1f mm  (%d counts)", distance, counts);
            ble_controller_notify_position(distance, counts);

            last_counts = counts;
        }

        // Task Delay
        vTaskDelay(TASK_DELAY_MS / portTICK_PERIOD_MS);
    }

    // Log unexpected exit
    ESP_LOGE(TAG, "Unexpected exit from main loop");
}