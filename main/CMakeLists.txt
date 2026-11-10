set(APP_SOURCES
    "humidity-sensor.c"
    "http/http.c"
    "http/http_ota.c"
    "storage/storage.c"
    "storage/ring-buffer.c"
    "network/network.c"
)

if(CONFIG_APP_QEMU)
    list(APPEND APP_SOURCES "sensors/sensors_mock.c")
    list(APPEND APP_SOURCES "network/network_qemu.c")
else()
    list(APPEND APP_SOURCES "sensors/sensors.c")
    list(APPEND APP_SOURCES "network/network_wifi.c")
endif()

idf_component_register(
    SRCS ${APP_SOURCES}
    INCLUDE_DIRS "." "sensors" "network" "http" "storage"
    REQUIRES
        app_update
        esp_app_format
        esp_psram
        esp_wifi
        esp_eth
        esp_event
        esp_timer
        esp_partition
        nvs_flash
        spi_flash
        esp_netif
        esp_http_server
        esp_https_server
    EMBED_TXTFILES
        "web/index.html.gz"
        "certs/servercert.pem"
        "certs/prvtkey.pem"
)