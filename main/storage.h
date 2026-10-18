#pragma once
#include <stdbool.h>
#include "sensor.h"

/* Non-fatal: failure only disables logging */
bool storage_init(void);
void storage_log(const char *state, const reading_t *inside, const reading_t *outside);
bool storage_ok(void);
