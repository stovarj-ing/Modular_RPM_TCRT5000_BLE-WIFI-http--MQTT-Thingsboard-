idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "mqtt_manager.c"
        "led_control.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        driver
        mqtt
        esp_event
        esp_wifi
        nvs_flash
        rpm_sensor
)