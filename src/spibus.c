#include "spibus.h"
#include "config.h"
#include "driver/spi_common.h"

esp_err_t spibus_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    return spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
}
