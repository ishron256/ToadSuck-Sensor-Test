idf_component_register(
    SRCS "app_main.c" "fans.c" "humidity.c" "sensor.c" "spibus.c" "storage.c" "web.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_wifi esp_netif esp_http_server esp_event nvs_flash fatfs sdmmc esp_timer
)
