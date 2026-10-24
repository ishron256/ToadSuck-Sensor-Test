#pragma once
#include <stdint.h>

/* Pins (ESP32-S3-WROOM-N4) */
#define PIN_FAN1        13   /* active high: N-FET/P-FET stage passes 12 V */
#define PIN_FAN2        14   /* active high: Fan 2 + external fans */
#define PIN_SNSR2        5   /* spare, unused */
#define PIN_VBUS_SENSE   4   /* via divider from 5 V VBUS */
#define PIN_SD_MISO     37
#define PIN_SD_SCK      36
#define PIN_SD_MOSI     35
#define PIN_SD_CS       15   /* from netlist: SD DAT3/CS is on GPIO15 (U2 pin 8) */
#define SD_CLOCK_KHZ    20000
#define SD_MOUNT_POINT  "/sdcard"

/* BME280 is on the same SPI bus as the SD card (netlist: U3) */
#define PIN_BME280_CS   10

/* Wi-Fi AP */
#define AP_SSID         "TSI_SmartVent"
#define AP_PASSWORD     "change-me-please"   /* TODO: real credentials, >= 8 chars */

/* Behaviour (all times are uptime since reset) */
#define STARTUP_FAN1_TEST_MS    (4u * 1000u)              /* confirmed */
#define OUTSIDE_SETTLE_MS       (12u * 60u * 1000u)
#define COLD_LOCKOUT_F          35.0f                     /* TODO confirm */
#define COLD_SLEEP_MS           (6u * 3600u * 1000u)
#define COLD_STIR_MS            (20u * 60u * 1000u)       /* confirmed */
#define FAN_RUN_MS              (106u * 60u * 1000u)
#define AH_DELTA_ON_GM3         (-0.5f)  /* confirmed: fans run when outside is >= 0.5 g/m3 drier than inside (inside - outside >= 0.5) */
#define WDT_FEED_PERIOD_MS      1000u
#define STATE_PROGRESS_MAX_MS   (COLD_SLEEP_MS + 10u * 60u * 1000u)
