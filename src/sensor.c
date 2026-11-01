#include "sensor.h"
#include "humidity.h"
#include "config.h"
#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* BME280 over SPI mode 0, forced mode, 1x oversampling, compensation per Bosch datasheet 4.2.3 */

static const char *TAG = "bme280";
static spi_device_handle_t s_dev;
static bool s_ready;

static struct {
    uint16_t T1; int16_t T2, T3;
    uint8_t H1, H3; int16_t H2, H4, H5; int8_t H6;
} c;

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
    uint8_t tx[32] = { reg | 0x80 }, rx[32];
    if (len > sizeof tx - 1) return ESP_ERR_INVALID_SIZE;
    spi_transaction_t t = { .length = (len + 1) * 8, .tx_buffer = tx, .rx_buffer = rx };
    esp_err_t e = spi_device_polling_transmit(s_dev, &t);
    if (e == ESP_OK) memcpy(buf, rx + 1, len);
    return e;
}

static esp_err_t reg_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg & 0x7F, val };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx };
    return spi_device_polling_transmit(s_dev, &t);
}

esp_err_t sensor_init(void)
{
    spi_device_interface_config_t dc = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_BME280_CS,
        .queue_size = 1,
    };
    esp_err_t e = spi_bus_add_device(SPI2_HOST, &dc, &s_dev);
    if (e != ESP_OK) return e;

    uint8_t id = 0;
    if ((e = reg_read(0xD0, &id, 1)) != ESP_OK) return e;
    if (id != 0x60) {
        ESP_LOGE(TAG, "unexpected chip id 0x%02X (is the sensor cable fitted?)", id);
        return ESP_ERR_NOT_FOUND;
    }

    reg_write(0xE0, 0xB6);   /* soft reset */
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t a[26], h[7];
    if ((e = reg_read(0x88, a, 26)) != ESP_OK) return e;
    if ((e = reg_read(0xE1, h, 7)) != ESP_OK) return e;
    c.T1 = a[0] | (a[1] << 8);
    c.T2 = (int16_t)(a[2] | (a[3] << 8));
    c.T3 = (int16_t)(a[4] | (a[5] << 8));
    c.H1 = a[25];
    c.H2 = (int16_t)(h[0] | (h[1] << 8));
    c.H3 = h[2];
    c.H4 = (int16_t)(((int8_t)h[3] * 16) | (h[4] & 0x0F));
    c.H5 = (int16_t)(((int8_t)h[5] * 16) | (h[4] >> 4));
    c.H6 = (int8_t)h[6];

    reg_write(0xF5, 0x00);   /* filter off */
    s_ready = true;
    return ESP_OK;
}

reading_t sensor_read(void)
{
    reading_t r = { .valid = false };
    if (!s_ready) return r;

    reg_write(0xF2, 0x01);   /* humidity x1 (must precede ctrl_meas) */
    reg_write(0xF4, 0x25);   /* temp x1, pressure skipped, forced mode */
    vTaskDelay(pdMS_TO_TICKS(15));

    uint8_t d[8];
    if (reg_read(0xF7, d, 8) != ESP_OK) return r;
    int32_t adc_t = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_h = ((int32_t)d[6] << 8) | d[7];
    if (adc_t == 0x80000 || adc_h == 0x8000) return r;   /* measurement skipped/not ready */

    double v1 = (adc_t / 16384.0 - c.T1 / 1024.0) * c.T2;
    double v2 = (adc_t / 131072.0 - c.T1 / 8192.0);
    v2 = v2 * v2 * c.T3;
    double t_fine = v1 + v2;
    double temp = t_fine / 5120.0;

    double x = t_fine - 76800.0;
    x = (adc_h - (c.H4 * 64.0 + c.H5 / 16384.0 * x)) *
        (c.H2 / 65536.0 * (1.0 + c.H6 / 67108864.0 * x * (1.0 + c.H3 / 67108864.0 * x)));
    x = x * (1.0 - c.H1 * x / 524288.0);
    if (x > 100.0) x = 100.0;
    if (x < 0.0) x = 0.0;

    if (temp < -40.0 || temp > 85.0) return r;
    r.temp_c = (float)temp;
    r.rh_pct = (float)x;
    r.ah_gm3 = humidity_abs_gm3(r.temp_c, r.rh_pct);
    r.valid = true;
    ESP_LOGI(TAG, "T=%.2f C  RH=%.1f %%  AH=%.2f g/m3", r.temp_c, r.rh_pct, r.ah_gm3);
    return r;
}
