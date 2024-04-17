// main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "ble_setup.h"
#include "ble_services.h"

#define GATTS_TAG "GATTS_DEMO"

void app_main() {
    esp_err_t ret;

    // Inicialización de NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicialización y configuración de Bluetooth
    ble_setup();

    // Registra los callbacks y inicia el servicio BLE
    ble_services_init();
}
