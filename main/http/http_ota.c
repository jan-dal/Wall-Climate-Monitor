#include "http_ota.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "freertos/task.h"

#include <stdint.h>
#include <string.h>

static const char *TAG = "HTTP OTA";

#define OTA_TOKEN CONFIG_APP_OTA_TOKEN

#define OTA_BUFFER_SIZE 1024

static void ota_restart_task(void *arg) {
    // Allow the HTTP response to reach the client.
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
}

esp_err_t ota_handler(httpd_req_t *req) {
    // 1. Check authorization before accepting firmware.
    char auth[128];

    if (httpd_req_get_hdr_value_str(req, "Authorization", auth, sizeof(auth)) != ESP_OK ||
        strncmp(auth, "Bearer ", 7) != 0 || strcmp(auth + 7, OTA_TOKEN) != 0) {

        httpd_resp_set_hdr(req, "WWW-Authenticate", "Bearer");
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
        return ESP_FAIL;
    }

    // 2. Find the inactive OTA partition.
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);

    if (partition == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition available");
        return ESP_FAIL;
    }

    // 3. Reject empty or oversized firmware images.
    if (req->content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty firmware image");
        return ESP_FAIL;
    }

    if (req->content_len > partition->size) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE,
                            "Firmware exceeds OTA partition size");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA target: %s, size: %u bytes", partition->label, (unsigned)req->content_len);

    // 4. Start the OTA operation.
    esp_ota_handle_t ota_handle;
    esp_err_t        err = esp_ota_begin(partition, req->content_len, &ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to start OTA");
        return ESP_FAIL;
    }

    // 5. Stream HTTP body directly into flash.
    uint8_t buffer[OTA_BUFFER_SIZE];
    size_t  remaining = req->content_len;

    while (remaining > 0) {
        size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);

        int received = httpd_req_recv(req, (char *)buffer, to_read);

        if (received <= 0) {
            ESP_LOGE(TAG, "Firmware upload interrupted: %d", received);
            esp_ota_abort(ota_handle);
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buffer, (size_t)received);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Firmware write failed");
            return ESP_FAIL;
        }

        remaining -= (size_t)received;
    }

    // 6. Finalize and validate the firmware image.
    err = esp_ota_end(ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Firmware validation failed");
        return ESP_FAIL;
    }

    // 7. Select the new firmware for the next boot.
    err = esp_ota_set_boot_partition(partition);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to select boot partition");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA successful, restarting");

    // 8. Restart asynchronously after sending a response.
    if (xTaskCreate(ota_restart_task, "ota_restart", 2048, NULL, 5, NULL) != pdPASS) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Update installed, but reboot scheduling failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);

    return httpd_resp_sendstr(req, "OTA successful. Restarting...\n");
}

const char *ota_state_name(const esp_partition_t *partition) {
    esp_ota_img_states_t state;

    if (partition == NULL || esp_ota_get_state_partition(partition, &state) != ESP_OK) {
        return "unknown";
    }

    switch (state) {
    case ESP_OTA_IMG_VALID:
        return "valid";
    case ESP_OTA_IMG_PENDING_VERIFY:
        return "pending_verify";
    case ESP_OTA_IMG_NEW:
        return "new";
    case ESP_OTA_IMG_UNDEFINED:
        return "undefined";
    case ESP_OTA_IMG_INVALID:
        return "invalid";
    case ESP_OTA_IMG_ABORTED:
        return "aborted";
    default:
        return "unknown";
    }
}