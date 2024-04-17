#ifndef BLE_SETUP_H
#define BLE_SETUP_H

#include "esp_err.h"

/**
 * @brief Inicializa el controlador Bluetooth y el stack Bluedroid.
 *
 * Esta función configura y habilita el controlador Bluetooth en modo BLE.
 * Además, inicializa y habilita Bluedroid, que es necesario para usar BLE.
 */
void ble_setup();

#endif // BLE_SETUP_H
