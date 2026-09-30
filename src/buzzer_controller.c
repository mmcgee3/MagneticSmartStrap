/**
 * @file buzzer_controller.c
 * 
 * @author Max McGee
 *
 * @brief Piezo Buzzer Controller. 
 * 
 * Controls a Piezo Buzzer, intended for use with a CPT-7502-65-SMT-TR using LEDC(PWM). 
 * Intializes the provided GPIO pin, functions to play simple tones, or a simple melody.
 */

/*
 * Copyright (c) 2026 Saturn Sports
 * All rights reserved.
 *
 * This firmware is proprietary and confidential. Unauthorized use, modification,
 * or distribution is prohibited without written permission.
 */
#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "buzzer_controller.h"

#define TAG "BUZZER_CTRL"

/**
 * @brief Initializes the piezo buzzer using LEDC (PWM).
 */
void buzzer_init(void){
    ledc_timer_config_t timer = {
        .speed_mode      = BUZZER_MODE,
        .timer_num       = BUZZER_TIMER,
        .duty_resolution = BUZZER_RES,
        .freq_hz         = BUZZER_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
 
    ledc_channel_config_t channel = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = BUZZER_MODE,
        .channel    = BUZZER_CHANNEL,
        .timer_sel  = BUZZER_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .duty       = BUZZER_DUTY_OFF,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
 
    ESP_LOGI(TAG, "Buzzer init: GPIO%d, %d Hz, %d-bit res, duty_on=%d",
             BUZZER_GPIO, BUZZER_FREQ_HZ, BUZZER_RES, BUZZER_DUTY_ON);
}

/**
 * @brief Sets the state of the buzzer (on/off).
 * @param state True to turn the buzzer on, false to turn it off.
 */
void buzzer_set_state(bool state){
    if (state) {
        ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_DUTY_ON));
        ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
        ESP_LOGI(TAG, "Buzzer ON");
    } else {
        ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_DUTY_OFF));
        ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
        ESP_LOGI(TAG, "Buzzer OFF");
    }
}

void play_phrase(void) {
    int freqs[] = {6600, 7000, 0, 6200, 6000}; // 0 = pause
    int times[] = {100, 100, 50, 100, 150};

    for (int i = 0; i < sizeof(freqs)/sizeof(freqs[0]); ++i) {
        if (freqs[i] == 0) {
            // Pause
            ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_DUTY_OFF));
        } else {
            // Set frequency and duty
            ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freqs[i]));
            ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_DUTY_ON));
        }
        ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
        vTaskDelay(times[i]/ portTICK_PERIOD_MS);
    }

    // Ensure buzzer is off after phrase
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
}