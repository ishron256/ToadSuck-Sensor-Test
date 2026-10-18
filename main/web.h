#pragma once
#include <stdbool.h>
#include "sensor.h"

void web_start(void);                        /* Wi-Fi AP + HTTP server */
void web_update(const char *state, const reading_t *inside, const reading_t *outside, bool fans_on);
