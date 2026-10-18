#pragma once

/* Absolute humidity in g/m^3 from temperature (C) and RH (%), Magnus formula */
float humidity_abs_gm3(float temp_c, float rh_pct);
float c_to_f(float c);
