idf_component_register(SRCS "lvgl_ui.c" "spi_lcd_main.c" "dv8_mqtt.c" "dv8_mqtt.h"
                       INCLUDE_DIRS "."
                       REQUIRES lvgl esp_lcd mqtt nvs_flash esp_event esp_netif json)
