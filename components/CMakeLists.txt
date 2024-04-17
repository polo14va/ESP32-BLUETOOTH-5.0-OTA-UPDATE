idf_component_register(
    SRCS "ble_services.c" "ble_setup.c" "nvs_service.c"
    INCLUDE_DIRS "." 
    REQUIRES bt nvs_flash
)
