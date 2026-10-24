#include "config.h"
#include "fans.h"
#include "humidity.h"
#include "sensor.h"
#include "spibus.h"
#include "storage.h"
#include "web.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "smartvent";

typedef enum {
    ST_STARTUP_TEST,   /* every reset: Fan 1 only, verify operation */
    ST_OUTSIDE_SETTLE, /* 12 min wait, fans off, then read "outside" air */
    ST_DECIDE,
    ST_FAN_RUN,        /* Fan 1 + Fan 2 (+ external) together */
    ST_COLD_SLEEP,     /* below 35F: fans off, recheck every 6 h */
    ST_COLD_STIR,
    ST_RETRY,          /* outside wetter: wait 5 min, re-read, decide again */
    ST_IDLE,
} state_t;

static const char *const NAMES[] = {
    "STARTUP_TEST", "OUTSIDE_SETTLE", "DECIDE", "FAN_RUN", "COLD_SLEEP", "COLD_STIR", "RETRY", "IDLE"
};

static state_t s_state = ST_STARTUP_TEST;
static int64_t s_state_since_ms;
static reading_t s_in, s_out;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static void enter(state_t s)
{
    ESP_LOGI(TAG, "-> %s", NAMES[s]);
    s_state = s;
    s_state_since_ms = now_ms();
}

static uint32_t elapsed(void) { return (uint32_t)(now_ms() - s_state_since_ms); }

static void step(void)
{
    switch (s_state) {
    case ST_STARTUP_TEST:
        fan1_set(true);
        if (elapsed() >= STARTUP_FAN1_TEST_MS) {
            s_in = sensor_read();   /* fan cavity holds inside air; assumption: startup test also takes the first inside reading */
            fans_all_off();
            enter(ST_OUTSIDE_SETTLE);
        }
        break;

    case ST_OUTSIDE_SETTLE:
        if (elapsed() >= OUTSIDE_SETTLE_MS) {
            s_out = sensor_read();   /* TODO: retry/timeout rules once defined */
            enter(ST_DECIDE);
        }
        break;

    case ST_DECIDE: {
        /* s_in was captured at the end of the last fan run (or startup test); s_out after the settle */
        if (!s_in.valid || !s_out.valid) { enter(ST_IDLE); break; }
        if (c_to_f(s_out.temp_c) < COLD_LOCKOUT_F) { enter(ST_COLD_SLEEP); break; }
        float delta = s_in.ah_gm3 - s_out.ah_gm3;   /* inside - outside */
        enter(delta >= -AH_DELTA_ON_GM3 ? ST_FAN_RUN : ST_RETRY);
        break;
    }

    case ST_RETRY:   /* outside wetter: re-read every 5 min, no new settle, no timeout */
        fans_all_off();
        if (elapsed() >= RETRY_MS) { s_out = sensor_read(); enter(ST_DECIDE); }
        break;

    case ST_FAN_RUN:
        fans_all_on();
        if (elapsed() >= FAN_RUN_MS) {
            s_in = sensor_read();   /* fan cavity is full of inside air at the end of the run */
            fans_all_off();
            enter(ST_OUTSIDE_SETTLE);
        }
        break;

    case ST_COLD_SLEEP:
        fans_all_off();
        if (elapsed() >= COLD_SLEEP_MS) enter(ST_COLD_STIR);
        break;

    case ST_COLD_STIR:
        fans_all_on();
        if (elapsed() >= COLD_STIR_MS) { fans_all_off(); enter(ST_OUTSIDE_SETTLE); }
        break;

    case ST_IDLE:
        fans_all_off();
        if (elapsed() >= COLD_SLEEP_MS) enter(ST_OUTSIDE_SETTLE);   /* TODO confirm idle recheck period */
        break;
    }
}

void app_main(void)
{
    fans_init();   /* first, so fans stay off */

    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_task_wdt_add(NULL);
    if (spibus_init() != ESP_OK) ESP_LOGE(TAG, "SPI bus init failed");
    if (sensor_init() != ESP_OK) ESP_LOGE(TAG, "BME280 init failed");
    storage_init();   /* failure is non-fatal */
    web_start();

    enter(ST_STARTUP_TEST);
    int64_t last_log = 0;

    for (;;) {
        step();
        esp_task_wdt_reset();

        if (elapsed() > STATE_PROGRESS_MAX_MS) {
            ESP_LOGE(TAG, "state %s stuck, restarting", NAMES[s_state]);
            fans_all_off();
            esp_restart();
        }

        web_update(NAMES[s_state], &s_in, &s_out, fan2_is_on());
        if (now_ms() - last_log > 60000) {
            last_log = now_ms();
            storage_log(NAMES[s_state], &s_in, &s_out);
        }
        vTaskDelay(pdMS_TO_TICKS(WDT_FEED_PERIOD_MS));
    }
}
