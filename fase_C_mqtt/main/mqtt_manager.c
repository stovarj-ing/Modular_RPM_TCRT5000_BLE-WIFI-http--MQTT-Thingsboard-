#include "mqtt_manager.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led_control.h"
#include "rpm_sensor.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_manager_config_t s_config;

static void publish_task(void *pvParameters)
{
    char payload[128];

    while (1)
    {
        float rpm = rpm_sensor_get_rpm();

        snprintf(payload, sizeof(payload),
         "{\"rpm\":%.2f}",
         rpm);
         
        int msg_id = esp_mqtt_client_publish(
            s_client,
            s_config.rpm_topic,
            payload,
            0,
            1,
            0
        );

        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "Published: %s", payload);
        }
        else
        {
            ESP_LOGW(TAG, "Publish failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(TAG, "MQTT CONNECTED");

            esp_mqtt_client_subscribe(
                s_client,
                s_config.led_topic,
                1
            );

            xTaskCreate(
                publish_task,
                "publish_task",
                4096,
                NULL,
                5,
                NULL
            );

            break;
        case MQTT_EVENT_PUBLISHED:

            ESP_LOGI(TAG, "MQTT MESSAGE PUBLISHED");

            break;

        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGW(TAG, "MQTT DISCONNECTED");

            break;

        case MQTT_EVENT_SUBSCRIBED:

            ESP_LOGI(TAG, "MQTT SUBSCRIBED");

            break;

        case MQTT_EVENT_DATA:
        {
            char topic[128] = {0};
            char data[128] = {0};

            memcpy(topic, event->topic, event->topic_len);
            memcpy(data, event->data, event->data_len);

            ESP_LOGI(TAG, "TOPIC=%s", topic);
            ESP_LOGI(TAG, "DATA=%s", data);

            // ThingsBoard RPC:
            // {"method":"setLed","params":true}

            if (strstr(data, "setLed") != NULL)
            {
                if (strstr(data, "true") != NULL)
                {
                    led_control_set(true);

                    ESP_LOGI(TAG, "LED ON");
                }
                else
                {
                    led_control_set(false);

                    ESP_LOGI(TAG, "LED OFF");
                }
            }

            break;
        }

        case MQTT_EVENT_ERROR:

            ESP_LOGE(TAG, "MQTT EVENT ERROR");

            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                ESP_LOGE(TAG,
                        "Last errno string (%s)",
                        strerror(event->error_handle->esp_transport_sock_errno));
            }

        break;

        default:
            break;
    }
}

esp_err_t mqtt_manager_init(const mqtt_manager_config_t *config)
{
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;

    return ESP_OK;
}

esp_err_t mqtt_manager_start(void)
{
    ESP_LOGI(TAG, "Starting MQTT...");

    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = s_config.broker_uri,

    .credentials = {
        .username = s_config.token,
        },

    .session = {
        .protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        },
    };
    ESP_LOGI(TAG, "Broker URI: %s", s_config.broker_uri);
    ESP_LOGI(TAG, "Token: %s", s_config.token);
    s_client = esp_mqtt_client_init(&mqtt_cfg);

    if (s_client == NULL)
    {
        ESP_LOGE(TAG, "MQTT client init failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT client initialized");

    esp_err_t err;

    err = esp_mqtt_client_register_event(
        s_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Register event failed");
        return err;
    }

    ESP_LOGI(TAG, "MQTT event registered");

    err = esp_mqtt_client_start(s_client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "MQTT start failed");
        return err;
    }

    ESP_LOGI(TAG, "MQTT start OK");

    return ESP_OK;
}