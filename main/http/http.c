#include "http.h"
#include "http_ota.h"
#include "sensors.h"
#include "storage/storage.h"

#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stddef.h>

#define CHUNK_SIZE 4096

static const char *TAG = "HTTP SERVER";

extern const char index_html_start[] asm("_binary_index_html_gz_start");

extern const char index_html_end[] asm("_binary_index_html_gz_end");

extern const unsigned char servercert_start[] asm("_binary_servercert_pem_start");

extern const unsigned char servercert_end[] asm("_binary_servercert_pem_end");

extern const unsigned char prvtkey_start[] asm("_binary_prvtkey_pem_start");

extern const unsigned char prvtkey_end[] asm("_binary_prvtkey_pem_end");

static httpd_handle_t http_server;
static httpd_handle_t redirect_server = NULL;

static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    return httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
}

static esp_err_t all_data_handler(httpd_req_t *req) {
    char   buffer[CHUNK_SIZE];
    size_t used = 0;

    measurement_data *data = NULL;
    size_t            records = 0;

    esp_err_t err = read_all_data(&data, &records);

    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not read stored data");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);

    buffer[used++] = '[';

    for (size_t i = 0; i < records; i++) {
        char json[192];

        int len = snprintf(json, sizeof(json),
                           "%s{\"timestamp\":%llu,"
                           "\"ds18b20_temperature\":%.2f,"
                           "\"aht20_temperature\":%.2f,"
                           "\"aht20_humidity\":%.2f}",
                           i == 0 ? "" : ",", (unsigned long long)data[i].timestamp,
                           (double)data[i].ds18b20_temperature, (double)data[i].aht20_temperature,
                           (double)data[i].aht20_humidity);

        if (len < 0 || (size_t)len >= sizeof(json)) {
            free(data);
            return ESP_FAIL;
        }

        if (used + len + 1 > CHUNK_SIZE) {
            err = httpd_resp_send_chunk(req, buffer, used);
            if (err != ESP_OK) {
                free(data);
                return err;
            }
            used = 0;
        }

        memcpy(buffer + used, json, len);
        used += len;
    }

    free(data);

    buffer[used++] = ']';

    err = httpd_resp_send_chunk(req, buffer, used);
    if (err != ESP_OK) {
        return err;
    }

    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t info_handler(httpd_req_t *req) {
    const esp_app_desc_t *app = esp_app_get_description();

    const esp_partition_t *partition = esp_ota_get_running_partition();

    if (app == NULL || partition == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Cannot read firmware information");
        return ESP_FAIL;
    }

    // Short build identifier from the embedded ELF SHA-256.
    char build_hash[17] = {0};
    esp_app_get_elf_sha256(build_hash, sizeof(build_hash));

    char json[256];

    int len = snprintf(json, sizeof(json),
                       "{"
                       "\"version\":\"%s\","
                       "\"build\":\"%s\","
                       "\"partition\":\"%s\","
                       "\"ota_state\":\"%s\""
                       "}",
                       app->version, build_hash, partition->label, ota_state_name(partition));

    if (len < 0 || (size_t)len >= sizeof(json)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Cannot format firmware information");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    return httpd_resp_send(req, json, len);
}

static esp_err_t redirect_handler(httpd_req_t *req) {
    char location[512];

    const char *https_origin = CONFIG_APP_HTTPS_DOMAIN;

    int written = snprintf(location, sizeof(location), "%s%s", https_origin, req->uri);

    if (written < 0 || written >= sizeof(location)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Redirect URL too long");
        return ESP_FAIL;
    }

    httpd_resp_set_status(req, "308 Permanent Redirect");
    httpd_resp_set_hdr(req, "Location", location);

    return httpd_resp_sendstr(req, "Redirecting to HTTPS...");
}

static const httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t all_data_uri = {
    .uri = "/api/data",
    .method = HTTP_GET,
    .handler = all_data_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t ota_uri = {
    .uri = "/api/ota",
    .method = HTTP_POST,
    .handler = ota_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t info_uri = {
    .uri = "/api/info",
    .method = HTTP_GET,
    .handler = info_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t redirect_uri = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = redirect_handler,
    .user_ctx = NULL,
};

void start_http_server(esp_netif_t *wifi_netif) {
    if (http_server != NULL) {
        return;
    }

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

    config.httpd.stack_size = 8192;
    config.httpd.recv_wait_timeout = 30;
    config.httpd.send_wait_timeout = 30;

    config.port_secure = 443;

    config.servercert = servercert_start;
    config.servercert_len = servercert_end - servercert_start;

    config.prvtkey_pem = prvtkey_start;
    config.prvtkey_len = prvtkey_end - prvtkey_start;

    struct ifreq ifr = {0};

    ESP_ERROR_CHECK(esp_netif_get_netif_impl_name(wifi_netif, ifr.ifr_name));

    ESP_LOGI(TAG, "HTTPS binding to %s", ifr.ifr_name);

    config.httpd.if_name = &ifr;

    ESP_ERROR_CHECK(httpd_ssl_start(&http_server, &config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &root_uri));

    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &all_data_uri));

    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &ota_uri));

    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &info_uri));

    // Small, separate HTTP server for redirects.
    httpd_config_t redirect_config = HTTPD_DEFAULT_CONFIG();

    redirect_config.server_port = 80;
    redirect_config.max_uri_handlers = 1;

    redirect_config.uri_match_fn = httpd_uri_match_wildcard;
    // Use the same network interface as HTTPS.
    redirect_config.if_name = &ifr;

    ESP_ERROR_CHECK(httpd_start(&redirect_server, &redirect_config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(redirect_server, &redirect_uri));

    ESP_LOGI(TAG, "HTTP redirect server started on port 80");
}