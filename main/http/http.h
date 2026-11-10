#pragma once

#include "esp_netif_types.h"

typedef struct {
    float *ds18b20_temperature;
    float *aht20_temperature;
    float *aht20_humidity;
} http_sensor_data_t;

void start_http_server(esp_netif_t *wifi_netif);