#include "wifi_manager.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi_manager";

typedef struct {
    wifi_manager_config_t config;
    EventGroupHandle_t event_group;
    esp_ip4_addr_t ip;
    uint32_t retry_count;
    bool initialized;
    bool connected;
} wifi_manager_state_t;

static wifi_manager_state_t s_wifi;

static void wifi_manager_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi.connected = false;
        if (s_wifi.retry_count < s_wifi.config.max_retry) {
            s_wifi.retry_count++;
            ESP_LOGW(TAG, "Retrying WiFi connection (%lu/%lu)",
                     (unsigned long)s_wifi.retry_count,
                     (unsigned long)s_wifi.config.max_retry);
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
        } else {
            xEventGroupSetBits(s_wifi.event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        s_wifi.ip = event->ip_info.ip;
        s_wifi.retry_count = 0;
        s_wifi.connected = true;
        ESP_LOGI(TAG, "Connected. IP: " IPSTR, IP2STR(&s_wifi.ip));
        xEventGroupSetBits(s_wifi.event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_init(const wifi_manager_config_t *config)
{
    if (config == NULL || config->ssid == NULL || config->password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_wifi.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_wifi.config = *config;
    if (s_wifi.config.max_retry == 0) {
        s_wifi.config.max_retry = WIFI_MANAGER_DEFAULT_MAX_RETRY;
    }
    if (s_wifi.config.connect_timeout_ms == 0) {
        s_wifi.config.connect_timeout_ms = WIFI_MANAGER_DEFAULT_TIMEOUT_MS;
    }

    s_wifi.event_group = xEventGroupCreate();
    if (s_wifi.event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_manager_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_manager_event_handler, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, s_wifi.config.ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, s_wifi.config.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode =
        strlen(s_wifi.config.password) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    s_wifi.initialized = true;
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void)
{
    if (!s_wifi.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xEventGroupClearBits(s_wifi.event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_wifi.retry_count = 0;
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi.event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(s_wifi.config.connect_timeout_ms));

    if ((bits & WIFI_CONNECTED_BIT) != 0) {
        return ESP_OK;
    }
    if ((bits & WIFI_FAIL_BIT) != 0) {
        return ESP_FAIL;
    }
    return ESP_ERR_TIMEOUT;
}

bool wifi_manager_is_connected(void)
{
    return s_wifi.connected;
}

esp_err_t wifi_manager_get_ip(esp_ip4_addr_t *ip)
{
    if (ip == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_wifi.connected) {
        return ESP_ERR_INVALID_STATE;
    }

    *ip = s_wifi.ip;
    return ESP_OK;
}
