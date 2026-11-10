#include "sensors.h"

#include "esp_check.h"
#include "onewire_bus.h" // IWYU pragma: keep

static const char *TAG = "SENSORS";

esp_err_t sensors_init(sensors_handles *handles) {
    ESP_RETURN_ON_FALSE(handles != NULL, ESP_ERR_INVALID_ARG, TAG, "Sensor handles can't be NULL");

    // ===================================================
    // ====================== BUSES ======================
    // ===================================================

    onewire_bus_handle_t onewire_bus = NULL;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_BUS_GPIO,
        .flags = {
            .en_pull_up = false, // enable the internal pull-up resistor in case the external device
                                 // didn't have one
        }};
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &onewire_bus));

    const i2c_config_t i2c_bus_conf = {.mode = I2C_MODE_MASTER,
                                       .sda_io_num = I2C_MASTER_SDA_IO,
                                       .sda_pullup_en = GPIO_PULLUP_ENABLE,
                                       .scl_io_num = I2C_MASTER_SCL_IO,
                                       .scl_pullup_en = GPIO_PULLUP_ENABLE,
                                       .master.clk_speed = I2C_MASTER_FREQ_HZ};
    i2c_bus_handle_t   i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &i2c_bus_conf);

    ESP_RETURN_ON_FALSE(i2c_bus != NULL, ESP_ERR_NOT_FOUND, TAG, "I2C bus can't be NULL");

    // =====================================================
    // ====================== DEVICES ======================
    // =====================================================
    ds18b20_config_t        ds18b20_config;
    ds18b20_device_handle_t ds18b20_handle = NULL;
    ESP_ERROR_CHECK(ds18b20_new_device_from_bus(onewire_bus, &ds18b20_config, &ds18b20_handle));

    aht20_i2c_config_t aht20_config = {
        .bus_inst = i2c_bus,
        .i2c_addr = AHT20_ADDRRES_0,
    };
    aht20_dev_handle_t aht20_handle = NULL;
    ESP_ERROR_CHECK(aht20_new_sensor(&aht20_config, &aht20_handle));

    handles->ds18b20_handle = ds18b20_handle;
    handles->aht20_handle = aht20_handle;

    return ESP_OK;
}

esp_err_t sensors_deinit(sensors_handles *handles) {
    ESP_RETURN_ON_FALSE(handles != NULL, ESP_ERR_INVALID_ARG, TAG, "Sensor handles can't be NULL");

    ESP_RETURN_ON_ERROR(aht20_del_sensor(handles->aht20_handle), TAG,
                        "AHT20 couldn't be deinitialized");
    ESP_RETURN_ON_ERROR(ds18b20_del_device(handles->ds18b20_handle), TAG,
                        "DS18B20 couldn't be deinitialized");

    return ESP_OK;
}

esp_err_t sensors_test(sensors_handles *handles) {
    ESP_RETURN_ON_FALSE(handles != NULL, ESP_ERR_INVALID_ARG, TAG, "Sensor handles can't be NULL");

    uint32_t temperature_raw, humidity_raw;
    float    temperature, humidity;

    ESP_RETURN_ON_ERROR(aht20_read_temperature_humidity(handles->aht20_handle, &temperature_raw,
                                                        &temperature, &humidity_raw, &humidity),
                        TAG, "AHT20 failed on test");

    ESP_RETURN_ON_ERROR(ds18b20_trigger_temperature_conversion(handles->ds18b20_handle), TAG,
                        "DS18B20 failed on test");

    return ESP_OK;
}

esp_err_t take_measurement(sensors_handles *handles, measurement_data *data) {
    uint32_t _temperature_raw, _humidity_raw;

    ESP_RETURN_ON_FALSE(handles != NULL, ESP_ERR_INVALID_ARG, TAG, "Sensor handles can't be NULL");

    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the measurement data can't be NULL");

    ESP_RETURN_ON_ERROR(ds18b20_trigger_temperature_conversion(handles->ds18b20_handle), TAG,
                        "Failed to start temperature measurement");

    ESP_RETURN_ON_ERROR(
        ds18b20_get_temperature(handles->ds18b20_handle, &(data->ds18b20_temperature)), TAG,
        "Failed to get DS18B20 temperature");

    ESP_RETURN_ON_ERROR(aht20_read_temperature_humidity(handles->aht20_handle, &_temperature_raw,
                                                        &(data->aht20_temperature), &_humidity_raw,
                                                        &(data->aht20_humidity)),
                        TAG, "Failed to get AHT20 measurements");

    data->timestamp = time(NULL);

    return ESP_OK;
}
