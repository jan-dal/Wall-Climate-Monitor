#include "sensors/sensors.h"
#include "storage/storage.h"
#include "network/network.h"

#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_timer.h"

#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define MEASUREMENT_PERIOD 60000000

static const char *TAG = "MAIN";

static sensors_handles  sensors = {0};
static sensors_handles *sensor_handles = &sensors;
static measurement_data data = {
    .ds18b20_temperature = 0,
    .aht20_temperature = 0,
    .aht20_humidity = 0,
    .timestamp = 0,
};

static void confirm_ota_firmware(void) {
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    const esp_partition_t *running = esp_ota_get_running_partition();

    esp_ota_img_states_t state;

    if (running == NULL || esp_ota_get_state_partition(running, &state) != ESP_OK ||
        state != ESP_OTA_IMG_PENDING_VERIFY) {
        return;
    }

    ESP_LOGI(TAG, "New firmware detected, waiting 30 seconds");

    vTaskDelay(pdMS_TO_TICKS(30000));

    // If the application crashes before reaching this point,
    // the bootloader will roll back on the next boot.
    ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());

    ESP_LOGI(TAG, "New firmware confirmed");
#endif
}

static void sensor_event_handler(void *) {
    time_t now = time(NULL);

    // Reject timestamps before 2025-01-01
    if (now < 1735689600) {
        ESP_LOGW(TAG, "Waiting for time synchronization...");
        return;
    }

    take_measurement(sensor_handles, &data);

    ESP_ERROR_CHECK(write_to_storage(data));

    ESP_LOGI(TAG,
             "Timestamp = %llu\n"
             "DS18B20 temperature=%.2f\n"
             "AHT20 temperature=%.2f\n"
             "AHT20 humidity=%.2f\n",
             data.timestamp, data.ds18b20_temperature, data.aht20_temperature, data.aht20_humidity);
}

void initialize_sensors() {
    sensors_init(sensor_handles);
    sensors_test(sensor_handles);

    esp_timer_create_args_t timer_config = {
        sensor_event_handler, NULL, ESP_TIMER_TASK, "Sensor timer", true,
    };

    esp_timer_handle_t timer_handle;
    esp_timer_create(&timer_config, &timer_handle);
    esp_timer_start_periodic(timer_handle, MEASUREMENT_PERIOD);
}

void app_main(void) {
    initialize_storage("storage");
    initialize_sensors();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(initialize_networking());

    confirm_ota_firmware();
}
