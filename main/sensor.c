#include "sensor.h"
#include "humidity.h"

/* TODO: BME280 driver over I2C (PIN_I2C_SDA/SCL, BME280_ADDR). Returns an
 * invalid reading until implemented; fill ah_gm3 with humidity_abs_gm3(). */
esp_err_t sensor_init(void) { return ESP_ERR_NOT_SUPPORTED; }

reading_t sensor_read(void)
{
    reading_t r = { .valid = false };
    return r;
}
