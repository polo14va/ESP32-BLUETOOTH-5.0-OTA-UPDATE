// nvs_service.h
#ifndef NVS_SERVICE_H
#define NVS_SERVICE_H

#include <stdint.h>
#include <stddef.h>

void initialize_nvs();
void save_characteristic_value(const char *key, uint8_t *value, size_t len);
void load_characteristic_value(const char *key, uint8_t *value, size_t *len);

#endif // NVS_SERVICE_H
