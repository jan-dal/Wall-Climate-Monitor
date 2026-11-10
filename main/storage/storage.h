#pragma once

#include "sensors.h"
#include "esp_err.h"

esp_err_t initialize_storage(const char *partition_label);
esp_err_t write_to_storage(measurement_data data);
esp_err_t read_all_data(measurement_data **data, size_t *records);