#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    float temp_c;
    float rh_pct;
    float ah_gm3;
    bool valid;
} reading_t;

esp_err_t sensor_init(void);
reading_t sensor_read(void);
