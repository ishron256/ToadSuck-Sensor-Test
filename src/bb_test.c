#ifdef BME280_BITBANG_TEST
#include "config.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Raw bit-banged SPI mode 0 read of BME280 ID (reg 0xD0, expect 0x60), bypassing the SPI driver. */

static const char *TAG = "bb_test";

static inline void dly(void) { esp_rom_delay_us(5); }

static uint8_t xfer(uint8_t out)
{
    uint8_t in = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(PIN_SD_MOSI, (out >> i) & 1);
        dly();
        gpio_set_level(PIN_SD_SCK, 1);
        dly();
        in = (uint8_t)((in << 1) | gpio_get_level(PIN_SD_MISO));
        gpio_set_level(PIN_SD_SCK, 0);
        dly();
    }
    return in;
}

static void out_pin(int pin, int level)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, level);
}

void bb_test_run(void)
{
    out_pin(PIN_SD_SCK, 0);
    out_pin(PIN_SD_MOSI, 0);
    out_pin(PIN_BME280_CS, 1);
    out_pin(PIN_SD_CS, 1);
    gpio_reset_pin(PIN_SD_MISO);
    gpio_set_direction(PIN_SD_MISO, GPIO_MODE_INPUT);

    for (;;) {
        ESP_LOGI(TAG, "idle (both CS high): MISO=%d", gpio_get_level(PIN_SD_MISO));

        gpio_set_level(PIN_BME280_CS, 0);
        dly();
        ESP_LOGI(TAG, "BME280 CS low, no clock: MISO=%d", gpio_get_level(PIN_SD_MISO));

        xfer(0xD0);
        uint8_t id = xfer(0x00);
        gpio_set_level(PIN_BME280_CS, 1);
        ESP_LOGI(TAG, "chip id = 0x%02X (expect 0x60)", id);

        gpio_set_level(PIN_BME280_CS, 0);
        dly();
        xfer(0xF3);
        uint8_t st = xfer(0x00);
        gpio_set_level(PIN_BME280_CS, 1);
        ESP_LOGI(TAG, "status reg 0xF3 = 0x%02X", st);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif
