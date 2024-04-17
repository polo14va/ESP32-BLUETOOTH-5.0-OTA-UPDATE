//main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "ble_services.h"

#define GATTS_SERVICE_UUID   0x00FF
#define GATTS_CHAR_UUID      0xFF01
#define GATTS_DESCR_UUID     0x3333
#define GATTS_NUM_HANDLE     4
#define GATTS_TAG "GATTS_DEMO"
#define PROFILE_APP_ID 0x00

void start_ble_advertising();

static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // First 4 bytes are service uuid
    0xab, 0xcd, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12,
    0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x00
};

static esp_gatt_char_prop_t a_property = 0;
static uint16_t service_handle;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static esp_attr_value_t char_val = {
    .attr_max_len = 500,  // Definir según la longitud máxima que esperas para el valor de la característica
    .attr_len = 1,        // Longitud inicial del valor
    .attr_value = (uint8_t[]) {0x00},  // Valor inicial, ajustar según sea necesario
};

static esp_gatt_srvc_id_t service_id = {
    .id = {
        .inst_id = 0,
        .uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = { .uuid16 = GATTS_SERVICE_UUID },
        },
    },
    .is_primary = true,
};

typedef struct {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
} gatts_profile_inst;

void gatts_callback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
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

        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT");
            // Aquí puedes manejar las solicitudes de lectura de los clientes
            break;

        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_WRITE_EVT");
            // Aquí se manejaría la escritura de datos del firmware
            break;

        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CREATE_EVT");
            service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(service_handle);
            esp_ble_gatts_add_char(service_handle, &((esp_bt_uuid_t){.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = GATTS_CHAR_UUID}}),
                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, char_prop_read_write_notify, &char_val, NULL);
            break;
            break;

        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_START_EVT");
            break;

        default:
            ESP_LOGI(GATTS_TAG, "Evento GATT no manejado %d", event);
            break;
    }
}


void gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
            break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
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

    uint8_t raw_adv_data[] = {
        0x02, 0x01, 0x06,  // Flagas: LE General Discoverable, BR/EDR not supported
        0x03, 0x03, 0xFF, 0x00  // 16-bit Service UUID
    };

    uint8_t raw_scan_rsp_data[] = {
        0x0F, 0x09, 'E', 'S', 'P', '3', '2', ' ', 'B', 'L', 'E', ' ', 'D', 'E', 'M', 'O'
    };

    esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
    esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));

    esp_ble_gap_start_advertising(&adv_params);
}

void app_main() {
    esp_err_t ret;

    // Inicialización de NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicialización de Bluetooth
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(GATTS_TAG, "%s init bluetooth\n", __func__);
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    // Registra los callbacks y inicia el servicio
    ret = esp_ble_gatts_register_callback(gatts_callback);
    ESP_ERROR_CHECK(ret);
    ret = esp_ble_gap_register_callback(gap_callback);
    ESP_ERROR_CHECK(ret);
    ret = esp_ble_gatts_app_register(PROFILE_APP_ID);
    ESP_ERROR_CHECK(ret);
    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler); // Aquí usas el manejador de servicios
    ESP_ERROR_CHECK(ret);

    start_ble_advertising();
}
