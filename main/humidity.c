#include "humidity.h"
#include <math.h>

float humidity_abs_gm3(float t, float rh)
{
    float es = 6.112f * expf(17.67f * t / (t + 243.5f));   /* hPa */
    return 216.7f * (rh / 100.0f * es) / (273.15f + t);
}

float c_to_f(float c) { return c * 9.0f / 5.0f + 32.0f; }
