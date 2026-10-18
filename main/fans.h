#pragma once
#include <stdbool.h>

void fans_init(void);          /* outputs low (fans off) */
void fan1_set(bool on);
void fan2_set(bool on);        /* also drives the external fans */
bool fan2_is_on(void);
void fans_all_off(void);
void fans_all_on(void);
