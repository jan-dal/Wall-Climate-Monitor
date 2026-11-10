#pragma once

#include <time.h>
#include "aht20.h"
#include "ds18b20.h"
#include "esp_err.h"

#define ONEWIRE_BUS_GPIO GPIO_NUM_4

#define I2C_MASTER_SCL_IO  GPIO_NUM_6 /*!< GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO  GPIO_NUM_5 /*!< GPIO number for I2C master data  */
#define I2C_MASTER_NUM     I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000     /*!< I2C master clock frequency */

typedef struct sensors_handles {
    ds18b20_device_handle_t ds18b20_handle;
    aht20_dev_handle_t      aht20_handle;
} sensors_handles;

typedef struct measurement_data {
    time_t timestamp;
    float  ds18b20_temperature;
    float  aht20_temperature;
    float  aht20_humidity;
} measurement_data;

esp_err_t sensors_init(sensors_handles *handles);
esp_err_t sensors_deinit(sensors_handles *handles);
esp_err_t sensors_test(sensors_handles *handles);
esp_err_t take_measurement(sensors_handles *handles, measurement_data *data);