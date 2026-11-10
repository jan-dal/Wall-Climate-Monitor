#include "network.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "http/http.h"

static const char *TAG = "NETWORK";

esp_err_t network_backend_init(void);

static void start_http_server_after_ip_init(void *arg, esp_event_base_t event_base,
                                            int32_t event_id, void *event_data) {
    if (event_base != IP_EVENT) {
        return;
    }

    if (event_id != IP_EVENT_STA_GOT_IP && event_id != IP_EVENT_ETH_GOT_IP) {
        return;
    }

    ip_event_got_ip_t *event = event_data;

    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

    start_http_server(event->esp_netif);
}

esp_err_t initialize_networking(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp_config));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               start_http_server_after_ip_init, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               start_http_server_after_ip_init, NULL));

    return network_backend_init();
}