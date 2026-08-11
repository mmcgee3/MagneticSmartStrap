/**
 * @file hall_sensor_controller.c
 * 
 * @author Max McGee
 *
 * @brief Quadrature Hall Effect Position Sensor Controller.
 * 
 * Controls a hall effect sensor based position sensor, using two hall effect sensors
 * placed at 90 degree phase shift from each other we can tell direction and number of 
 * sensors that have passed.
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
#include "driver/pulse_cnt.h"

#include "app_config.h"
#include "hall_sensor_controller.h"

#define TAG "HALL_CTRL"

static pcnt_unit_handle_t s_unit = NULL;

/**
 * @brief Initializes the two Hall sensors as a PCNT quadrature decoder.
 */
void hall_sensor_init(void){
    pcnt_unit_config_t unit = {
        .high_limit =  HALL_COUNT_LIMIT,
        .low_limit  = -HALL_COUNT_LIMIT,
    };

    ESP_ERROR_CHECK(pcnt_new_unit(&unit, &s_unit));

    pcnt_glitch_filter_config_t filter = { .max_glitch_ns = HALL_GLITCH_NS };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_unit, &filter));

    // Channel A counts edges on A, reads B's level to decide direction
    pcnt_chan_config_t chan_a_cfg = { .edge_gpio_num = HALL_GPIO_A, .level_gpio_num = HALL_GPIO_B };
    pcnt_channel_handle_t chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(s_unit, &chan_a_cfg, &chan_a));

    // Channel B counts edges on B, reads A's level to decide direction
    pcnt_chan_config_t chan_b_cfg = { .edge_gpio_num = HALL_GPIO_B, .level_gpio_num = HALL_GPIO_A };
    pcnt_channel_handle_t chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(s_unit, &chan_b_cfg, &chan_b));

    // Quadrate, count on every edge direction of both channels
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
 
    ESP_ERROR_CHECK(pcnt_unit_enable(s_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(s_unit));
 
    ESP_LOGI(TAG, "Hall init: A=GPIO%d, B=GPIO%d, pitch=%.1fmm",
             HALL_GPIO_A, HALL_GPIO_B, MAGNET_PITCH_MM);
}

/**
 * @brief Returns the signed quadrature count (+ = A leads B, - = B leads A).
 */
int hall_sensor_get_counts(void){
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(s_unit, &count));
    return count;
}
 
/**
 * @brief Returns net travel in mm. Each count is a quarter of the magnet pitch.
 */
float hall_sensor_get_distance_mm(void){
    return hall_sensor_get_counts() * (MAGNET_PITCH_MM / COUNTS_PER_MAGNET);
}
 
/**
 * @brief Zeroes the count. Call while the strap is stationary.
 */
void hall_sensor_reset(void){
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_unit));
}