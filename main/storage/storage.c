#include "storage/storage.h"
#include "sensors.h"
#include "storage/ring-buffer.h"
#include <stdint.h>

static const char *TAG = "STORAGE";

esp_err_t initialize_storage(const char *partition_label) {
    return ring_buffer_init(partition_label, sizeof(measurement_data));
}

esp_err_t write_to_storage(measurement_data data) {
    return ring_buffer_write((const uint8_t *)&data);
}

esp_err_t read_all_data(measurement_data **data, size_t *records) {
    if (data == NULL || records == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *data = NULL;
    *records = 0;

    uint8_t *buffer = NULL;
    size_t   buffer_size = 0;

    esp_err_t err = ring_buffer_read_all(&buffer, &buffer_size);
    if (err != ESP_OK) {
        return err;
    }

    if (buffer_size % sizeof(measurement_data) != 0) {
        free(buffer);
        return ESP_ERR_INVALID_SIZE;
    }

    *data = (measurement_data *)buffer;
    *records = buffer_size / sizeof(measurement_data);

    return ESP_OK;
}