#include "sensors.h"

static bool     initialized = false;
static uint32_t sample_number = 0;

esp_err_t sensors_init(sensors_handles *handles) {
    if (handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    handles->ds18b20_handle = NULL;
    handles->aht20_handle = NULL;

    sample_number = 0;
    initialized = true;

    return ESP_OK;
}

esp_err_t sensors_deinit(sensors_handles *handles) {
    if (handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    handles->ds18b20_handle = NULL;
    handles->aht20_handle = NULL;

    initialized = false;

    return ESP_OK;
}

esp_err_t sensors_test(sensors_handles *handles) {
    if (handles == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return initialized ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t take_measurement(sensors_handles *handles, measurement_data *data) {
    if (handles == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    data->ds18b20_temperature = 18.0f + (sample_number % 50) * 0.1f;

    data->aht20_temperature = 19.0f + (sample_number % 40) * 0.1f;

    data->aht20_humidity = 40.0f + (sample_number % 30) * 0.5f;

    data->timestamp = time(NULL);

    sample_number++;

    return ESP_OK;
}