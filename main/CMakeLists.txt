idf_component_register(SRCS "led_wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES "led_controller" "wifi_manager" "http_server" "log" "nvs_flash" "esp_wifi" "esp_driver_rmt")
