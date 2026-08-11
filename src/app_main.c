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

#include "app_config.h" 
#include "ble_controller.h"
#include "buzzer_controller.h"
#include "hall_sensor_controller.h"

#define TASK_DELAY_MS 50

#define TAG "APP_MAIN"


void app_main(void){
    ESP_LOGI(TAG, "Application starting up!");
    
    // INIT BLE
    buzzer_init();
    hall_sensor_init();

    hall_sensor_reset();

    // Main Loop
    while(1) {
        float distance = hall_sensor_get_distance_mm();
        int counts = hall_sensor_get_counts();
        
        ESP_LOGI("app", "%.1f mm  (%d counts)", distance, counts);

        // Task Delay
        vTaskDelay(TASK_DELAY_MS / portTICK_PERIOD_MS);
    }

    // Log unexpected exit
    ESP_LOGE(TAG, "Unexpected exit from main loop");
}