/*
 * Copyright (c) 2026 Saturn Sports
 * All rights reserved.
 *
 * This firmware is proprietary and confidential. Unauthorized use, modification,
 * or distribution is prohibited without written permission.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "app_config.h" 
#include "ble_controller.h"
#include "buzzer_controller.h"
#include "hall_sensor_controller.h"

#define TASK_DELAY_MS 50    // Period between loop runs

#define TAG_MAIN "APP_MAIN"


void app_main(void){
    ESP_LOGI(TAG_MAIN, "Application starting up!");
    
    // INIT BLE
    buzzer_init();
    // INIT HALL SENSORS

    // Main Loop
    while(1) {

        for (int i = 0; i < NUM_SENSORS ; i++){
            // CHECK EACH SENSOR
        }

        // Task Delay
        vTaskDelay(TASK_DELAY_MS / portTICK_PERIOD_MS);
    }

    // Log unexpected exit
    ESP_LOGE(TAG_MAIN, "Unexpected exit from main loop");
}
