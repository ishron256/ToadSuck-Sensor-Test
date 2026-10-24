#pragma once
#include "esp_err.h"

/* Shared SPI2 bus (SD card + BME280). Call once before sensor_init/storage_init. */
esp_err_t spibus_init(void);
