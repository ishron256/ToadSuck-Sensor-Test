#include "storage.h"
#include "config.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

static const char *TAG = "storage";
static bool s_ok;

bool storage_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    if (spi_bus_initialize(SPI2_HOST, &bus, SDSPI_DEFAULT_DMA) != ESP_OK) {
        ESP_LOGW(TAG, "SPI bus init failed");
        return false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = SD_CLOCK_KHZ;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = PIN_SD_CS;
    slot.host_id = SPI2_HOST;

    esp_vfs_fat_mount_config_t mc = { .max_files = 3, .format_if_mount_failed = false };
    sdmmc_card_t *card;
    esp_err_t e = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot, &mc, &card);
    s_ok = (e == ESP_OK);
    if (!s_ok) ESP_LOGW(TAG, "SD mount failed: %s (continuing without logging)", esp_err_to_name(e));
    return s_ok;
}

bool storage_ok(void) { return s_ok; }

void storage_log(const char *state, const reading_t *in, const reading_t *out)
{
    if (!s_ok) return;
    FILE *f = fopen(SD_MOUNT_POINT "/log.csv", "a");
    if (!f) { s_ok = false; return; }   /* stop trying after a write failure */
    fprintf(f, "%lu,%s,%.2f,%.1f,%.2f,%.2f,%.1f,%.2f\n",
            (unsigned long)esp_log_timestamp(), state,
            in->temp_c, in->rh_pct, in->ah_gm3,
            out->temp_c, out->rh_pct, out->ah_gm3);
    fclose(f);
}
