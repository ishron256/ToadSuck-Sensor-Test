#include "fans.h"
#include "config.h"
#include "driver/gpio.h"

static bool s_fan2;

void fans_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_FAN1) | (1ULL << PIN_FAN2),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);
    gpio_set_level(PIN_FAN1, 0);
    gpio_set_level(PIN_FAN2, 0);
    s_fan2 = false;
}

void fan1_set(bool on) { gpio_set_level(PIN_FAN1, on); }
void fan2_set(bool on) { s_fan2 = on; gpio_set_level(PIN_FAN2, on); }
bool fan2_is_on(void)  { return s_fan2; }
void fans_all_off(void) { fan1_set(false); fan2_set(false); }
void fans_all_on(void)  { fan1_set(true);  fan2_set(true); }
