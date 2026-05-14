#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "rpm_sensor.h"
#include "web_server.h"
#include "wifi_manager.h"

/* Ajusta estas macros para tu red y tu cableado real. */
#define WIFI_SSID     "TU_SSID"
#define WIFI_PASSWORD "TU_PASSWORD"

#define RPM_SENSOR_GPIO             GPIO_NUM_4
#define RPM_SENSOR_PULSES_PER_REV   1U
#define RPM_SENSOR_SAMPLE_PERIOD_MS 1000U
#define RPM_SENSOR_DEBOUNCE_US      1000U

#define LED_GPIO        GPIO_NUM_8
#define LED_ACTIVE_HIGH true

static const char *TAG = "fase_b";

static esp_err_t app_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    return err;
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_init_nvs());

    const rpm_sensor_config_t rpm_config = {
        .gpio_num = RPM_SENSOR_GPIO,
        .pulses_per_revolution = RPM_SENSOR_PULSES_PER_REV,
        .sample_period_ms = RPM_SENSOR_SAMPLE_PERIOD_MS,
        .debounce_us = RPM_SENSOR_DEBOUNCE_US,
        .interrupt_type = GPIO_INTR_POSEDGE,
        .enable_pullup = true,
        .enable_pulldown = false,
    };

    ESP_ERROR_CHECK(rpm_sensor_init(&rpm_config));
    ESP_ERROR_CHECK(rpm_sensor_start());

    const wifi_manager_config_t wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .max_retry = WIFI_MANAGER_DEFAULT_MAX_RETRY,
        .connect_timeout_ms = WIFI_MANAGER_DEFAULT_TIMEOUT_MS,
    };

    ESP_ERROR_CHECK(wifi_manager_init(&wifi_config));
    esp_err_t err = wifi_manager_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed: %s", esp_err_to_name(err));
        return;
    }

    const web_server_config_t web_config = {
        .led_gpio = LED_GPIO,
        .led_active_high = LED_ACTIVE_HIGH,
    };

    ESP_ERROR_CHECK(web_server_init(&web_config));
    ESP_ERROR_CHECK(web_server_start());

    ESP_LOGI(TAG, "Fase B lista. Abre http://<IP_DEL_ESP32>/ en el navegador");
}
