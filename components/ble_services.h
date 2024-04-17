#ifndef BLE_SERVICES_H
#define BLE_SERVICES_H

#include "esp_gatts_api.h"

void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void ble_services_init() ;

#endif // BLE_SERVICES_H
