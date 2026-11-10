#include "esp_wifi.h"
#include "esp_check.h"
#include "mdns.h"
#include "sdkconfig.h"

static const char *TAG = "WIFI";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
    if (event_base != WIFI_EVENT) {
        return;
    }

    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event = event_data;

        if (event->reason == WIFI_REASON_ROAMING) {
            ESP_LOGI(TAG, "Wi-Fi roaming in progress");
            return;
        }

        ESP_LOGW(TAG, "Wi-Fi disconnected, reason: %d", event->reason);

        esp_err_t err = esp_wifi_connect();

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Wi-Fi reconnect failed: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t network_backend_init(void) {
    ESP_RETURN_ON_FALSE(esp_netif_create_default_wifi_sta() != NULL, ESP_FAIL, TAG,
                        "Failed to create Wi-Fi network interface");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(CONFIG_APP_MDNS_DOMAIN));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Sensor"));

    mdns_service_add("ESP32 HTTPS Server", "_http", "_tcp", 80, NULL, 0);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_cfg = {
        .sta =
            {
                .ssid = CONFIG_APP_WIFI_SSID,
                .password = CONFIG_APP_WIFI_PASSWORD,

                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,

                // Enable network-assisted roaming.
                .rm_enabled = 1,  // 802.11k
                .btm_enabled = 1, // 802.11v
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                               wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    return esp_wifi_connect();
}
