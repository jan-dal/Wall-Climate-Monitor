#include <assert.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_eth.h" // IWYU pragma: keep
#include "esp_netif.h"

static const char *TAG = "QEMU NETWORK";

esp_err_t network_backend_init(void) {
    ESP_LOGI(TAG, "Initializing QEMU Ethernet");

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t       *netif = esp_netif_new(&netif_config);
    assert(netif != NULL);

    // Configure QEMU Ethernet.
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.reset_gpio_num = -1;
    phy_config.autonego_timeout_ms = 100;

    esp_eth_mac_t *mac = esp_eth_mac_new_openeth(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);

    assert(mac != NULL);
    assert(phy != NULL);

    // Install Ethernet driver.
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);

    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

    uint8_t mac_addr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};

    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));

    // Attach Ethernet to the TCP/IP stack.
    esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(eth_handle);

    assert(glue != NULL);

    ESP_ERROR_CHECK(esp_netif_attach(netif, glue));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG, "QEMU Ethernet initialized");

    return ESP_OK;
}
