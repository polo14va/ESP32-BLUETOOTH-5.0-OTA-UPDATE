// nvs_service.c
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define GATTS_TAG "NVS_SERVICES"
#define STORAGE_NAMESPACE "storage"

void initialize_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(GATTS_TAG, "NVS initialized successfully!");
}

void save_characteristic_value(const char *key, uint8_t *value, size_t len) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        return;
    } else {
        ret = nvs_set_blob(my_handle, key, value, len);
        if (ret != ESP_OK) {
            ESP_LOGE(GATTS_TAG, "Failed to set blob! %s", esp_err_to_name(ret));
        } else {
            ret = nvs_commit(my_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(GATTS_TAG, "Failed to commit changes! %s", esp_err_to_name(ret));
            }
        }
        nvs_close(my_handle);
    }
}



void load_characteristic_value(const char *key, uint8_t *value, size_t *len) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TAG, "Error (%s) opening NVS handle for read!", esp_err_to_name(ret));
        return;
    } else {
        ret = nvs_get_blob(my_handle, key, value, len);
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(GATTS_TAG, "The value is not initialized yet!");
        } else if (ret != ESP_OK) {
            ESP_LOGE(GATTS_TAG, "Error (%s) reading!", esp_err_to_name(ret));
        } else {
            ESP_LOGI(GATTS_TAG, "Read successful, value: %.*s", (int)*len, value);
        }
        nvs_close(my_handle);
    }
}


