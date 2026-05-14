#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "rpm_sensor.h"
#include "led_control.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"

#define WIFI_SSID     "UNAL-IoT-2.4"
#define WIFI_PASSWORD "banano2026"

#define MQTT_BROKER_URI "mqtt://192.168.1.162:1883"
#define MQTT_RPM_TOPIC  "v1/devices/me/telemetry"
#define MQTT_LED_TOPIC  "v1/devices/me/rpc/request/+"
#define THINGSBOARD_TOKEN "sebas123"

#define RPM_SENSOR_GPIO             GPIO_NUM_4
#define RPM_SENSOR_PULSES_PER_REV   1U
#define RPM_SENSOR_SAMPLE_PERIOD_MS 1000U
#define RPM_SENSOR_DEBOUNCE_US      1000U

#define LED_GPIO        GPIO_NUM_8
#define LED_ACTIVE_HIGH true

static const char *TAG = "fase_C_mqtt";

static esp_err_t init_nvs(void)
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
    ESP_ERROR_CHECK(init_nvs());

    const rpm_sensor_config_t rpm_config = {
        .gpio_num = RPM_SENSOR_GPIO,
        .pulses_per_revolution = RPM_SENSOR_PULSES_PER_REV,
        .sample_period_ms = RPM_SENSOR_SAMPLE_PERIOD_MS,
        .debounce_us = RPM_SENSOR_DEBOUNCE_US,
        .interrupt_type = GPIO_INTR_POSEDGE,
        .enable_pullup = true,
    };
    ESP_ERROR_CHECK(rpm_sensor_init(&rpm_config));
    ESP_ERROR_CHECK(rpm_sensor_start());

    const led_control_config_t led_config = {
        .gpio_num = LED_GPIO,
        .active_high = LED_ACTIVE_HIGH,
    };
    ESP_ERROR_CHECK(led_control_init(&led_config));

    const wifi_manager_config_t wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .max_retry = WIFI_MANAGER_DEFAULT_MAX_RETRY,
        .connect_timeout_ms = WIFI_MANAGER_DEFAULT_TIMEOUT_MS,
    };
    ESP_ERROR_CHECK(wifi_manager_init(&wifi_config));
    ESP_ERROR_CHECK(wifi_manager_connect());

    const mqtt_manager_config_t mqtt_config = {
        .broker_uri = MQTT_BROKER_URI,
        .rpm_topic = MQTT_RPM_TOPIC,
        .led_topic = MQTT_LED_TOPIC,
        .token = THINGSBOARD_TOKEN,
    };
    ESP_ERROR_CHECK(mqtt_manager_init(&mqtt_config));
    ESP_ERROR_CHECK(mqtt_manager_start());
    


    ESP_LOGI(TAG, "Fase C base lista");
}
