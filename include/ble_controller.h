#define _BLE_CONTROLLER_H_

#include <stdint.h>
#include <stdbool.h>

void ble_controller_init(void);
bool ble_controller_is_connected(void);
void ble_controller_notify_position(float distance_mm, int32_t counts);