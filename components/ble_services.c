// ble_services.c

#include <string.h>
#include "esp_log.h"
#include "esp_gatts_api.h"
#include "ble_services.h"
#include "esp_gap_ble_api.h"
#include "nvs_service.h"

/* SERVICIOS */
#define GATTS_SERVICE_UUID   0x00FF
#define GATTS_CHAR_UUID      0xFF01

#define OTA_SERVICE_UUID     0xB200
#define OTA_CHAR_UUID        0xB201
/* FIN SERVICIOS*/

#define GATTS_DESCR_UUID     0x3333
#define GATTS_NUM_HANDLE     4
#define GATTS_TAG "BLE_SERVICES"
#define PROFILE_APP_ID 0x00 

void start_ble_advertising();



/* SERVICIOS */
static uint16_t service_handle;
static const esp_gatt_srvc_id_t service_id = {
    .is_primary = true,
    .id = {
        .inst_id = 0,
        .uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID},
        }
    }
};

static uint16_t ota_service_handle;
static const esp_gatt_srvc_id_t ota_service_id = {
    .is_primary = true,
    .id = {
        .inst_id = 0,
        .uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = OTA_SERVICE_UUID},
        }
    }
};
/* FIN SERVICIOS*/

/* CARACTERISTICAS SERVICIOS*/
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static esp_attr_value_t char_val = {
    .attr_max_len = 500,
    .attr_len = 1,
    .attr_value = (uint8_t[]) {0x00},
};

// Características del nuevo servicio
static const uint8_t ota_char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static esp_attr_value_t ota_char_val = {
    .attr_max_len = 500,
    .attr_len = 1, // El tamaño inicial puede ser 0 o 1 según lo prefieras
    .attr_value = (uint8_t[]) {0x00}, // Valor inicial
};
/* FIN CARACTERISTICAS SERVICIOS*/

uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,  // Flagas: LE General Discoverable, BR/EDR not supported
    0x05, 0x03, 0xFF, 0x00, 0xB2, 0x00  // Los 2 servicios que se anunciarán
};

uint8_t raw_scan_rsp_data[] = {
    0x0F, 0x09, 'E', 'S', 'P', '3', '2', ' ', 'B', 'L', 'E', ' ', 'D', 'E', 'M', 'O'
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


void initialize_default_values() {
    char_val.attr_max_len = 500;
    char_val.attr_len = 4; // Ajusta esto según tu necesidad
    memcpy(char_val.attr_value, (uint8_t[]) {0xde, 0xad, 0xbe, 0xef}, 4);

    ota_char_val.attr_max_len = 500;
    ota_char_val.attr_len = 4; // Ajusta esto según tu necesidad
    memcpy(ota_char_val.attr_value, (uint8_t[]) {0x01, 0x02, 0x03, 0x04}, 4);
}


void gatts_callback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    static bool general_service_created = false;

    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_REG_EVT");
            esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT");
            // Aquí puedes guardar el gatt_if y el ID de conexión, que podrías necesitar para operaciones futuras
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT");
            start_ble_advertising();
            break;

        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT, handle: %d", param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            size_t len = sizeof(rsp.attr_value.value);
            if (param->read.handle == (service_handle + 1)) {
                load_characteristic_value("char_val", rsp.attr_value.value, &len);
                rsp.attr_value.len = len;
            } else if (param->read.handle == (ota_service_handle + 1)) {
                load_characteristic_value("ota_char_val", rsp.attr_value.value, &len);
                rsp.attr_value.len = len;
            }
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(GATTS_TAG, "Write Event, Handle: %d, Value: %.*s", param->write.handle, param->write.len, param->write.value);
            if (param->write.handle == (service_handle + 1)) {
                memcpy(char_val.attr_value, param->write.value, param->write.len);
                char_val.attr_len = param->write.len;
                save_characteristic_value("char_val", char_val.attr_value, char_val.attr_len);
            } else if (param->write.handle == (ota_service_handle + 1)) {
                memcpy(ota_char_val.attr_value, param->write.value, param->write.len);
                ota_char_val.attr_len = param->write.len;
                save_characteristic_value("ota_char_val", ota_char_val.attr_value, ota_char_val.attr_len);
            }
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            break;
        }

        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CREATE_EVT");
            if (param->create.status == ESP_GATT_OK) {
                if (!general_service_created) {
                    service_handle = param->create.service_handle;
                    esp_ble_gatts_start_service(service_handle);
                    esp_ble_gatts_add_char(service_handle, &((esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID}}),
                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, char_prop_read_write_notify, &char_val, NULL);
                    // Indicar que el servicio general se ha creado
                    general_service_created = true;
                    // Ahora crear el servicio OTA
                    esp_ble_gatts_create_service(gatts_if, &ota_service_id, GATTS_NUM_HANDLE);
                } else {
                    ota_service_handle = param->create.service_handle;
                    esp_ble_gatts_start_service(ota_service_handle);
                    esp_ble_gatts_add_char(ota_service_handle, &((esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = OTA_CHAR_UUID}}),
                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, ota_char_prop_read_write_notify, &ota_char_val, NULL);
                }
            }
            break;

        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_START_EVT");
            break;

        default:
            ESP_LOGI(GATTS_TAG, "Evento GATT no manejado %d", event);
            break;
    }
}

static void gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(GATTS_TAG, "Scan started successfully");
            } else {
                ESP_LOGE(GATTS_TAG, "Failed to start scan process");
            }
            break;
        default:
            ESP_LOGI(GATTS_TAG, "Evento GAP no manejado %d", event);
            break;
    }
}

void start_ble_advertising() {
    esp_ble_adv_params_t adv_params = {
        .adv_int_min       = 0x20,
        .adv_int_max       = 0x40,
        .adv_type          = ADV_TYPE_IND,
        .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
        .channel_map       = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    uint8_t raw_scan_rsp_data[] = {
        0x0F, 0x09, 'E', 'S', 'P', '3', '2', ' ', 'B', 'L', 'E', ' ', 'D', 'E', 'M', 'O'
    };

    esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
    esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
   
    esp_ble_gap_start_advertising(&adv_params);
}

void ble_services_init() {
    esp_ble_gatts_register_callback(gatts_callback);
    esp_ble_gap_register_callback(gap_callback);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
    initialize_nvs();
    initialize_default_values();
    start_ble_advertising();
}


