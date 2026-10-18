#include "web.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* Served entirely from RAM so the page works without the SD card */
static struct { char state[24]; reading_t in, out; bool fans; } s_snap;
static SemaphoreHandle_t s_lock;

void web_update(const char *state, const reading_t *in, const reading_t *out, bool fans_on)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(s_snap.state, state, sizeof s_snap.state);
    s_snap.in = *in; s_snap.out = *out; s_snap.fans = fans_on;
    xSemaphoreGive(s_lock);
}

static esp_err_t root_get(httpd_req_t *req)
{
    char buf[512];
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int n = snprintf(buf, sizeof buf,
        "<html><body><h1>TSI SmartVent V3</h1>"
        "<p>State: %s | Fans: %s</p>"
        "<p>Inside: %.1fC %.0f%% %.2f g/m3</p>"
        "<p>Outside: %.1fC %.0f%% %.2f g/m3</p>"
        "</body></html>",
        s_snap.state, s_snap.fans ? "ON" : "OFF",
        s_snap.in.temp_c, s_snap.in.rh_pct, s_snap.in.ah_gm3,
        s_snap.out.temp_c, s_snap.out.rh_pct, s_snap.out.ah_gm3);
    xSemaphoreGive(s_lock);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, buf, n);
}

void web_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    strcpy(s_snap.state, "BOOT");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t ap = { .ap = {
        .ssid = AP_SSID, .ssid_len = sizeof(AP_SSID) - 1,
        .password = AP_PASSWORD, .max_connection = 4,
        .authmode = WIFI_AUTH_WPA2_PSK,
    } };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap);
    esp_wifi_start();

    httpd_handle_t h = NULL;
    httpd_config_t hc = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&h, &hc) == ESP_OK) {
        httpd_uri_t u = { .uri = "/", .method = HTTP_GET, .handler = root_get };
        httpd_register_uri_handler(h, &u);
    }
}
