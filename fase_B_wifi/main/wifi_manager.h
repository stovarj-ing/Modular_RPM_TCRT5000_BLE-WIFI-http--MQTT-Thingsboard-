#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_netif_ip_addr.h"

#define WIFI_MANAGER_DEFAULT_MAX_RETRY 5U
#define WIFI_MANAGER_DEFAULT_TIMEOUT_MS 15000U

typedef struct {
    const char *ssid;
    const char *password;
    uint32_t max_retry;
    uint32_t connect_timeout_ms;
} wifi_manager_config_t;

esp_err_t wifi_manager_init(const wifi_manager_config_t *config);
esp_err_t wifi_manager_connect(void);
bool wifi_manager_is_connected(void);
esp_err_t wifi_manager_get_ip(esp_ip4_addr_t *ip);

#endif
