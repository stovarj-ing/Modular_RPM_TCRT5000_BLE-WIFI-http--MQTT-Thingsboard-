idf_component_register(
    SRCS
        "main.c"
        "ble_server.c"
        "led_control.c"
    INCLUDE_DIRS "."
    REQUIRES
        bt
        esp_driver_gpio
        nvs_flash
        rpm_sensor
)
