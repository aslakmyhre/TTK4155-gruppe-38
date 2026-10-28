#include "controller.h"

#define KP      2.0f        // percent speed per percent error
#define KI      1.0f        // percent speed per (percent error * s)
#define T       (CONTROLLER_PERIOD_MS / 1000.0f)
#define U_MAX   100.0f

static float track_counts = 1.0f;
static float integral = 0.0f;

void controller_init(int32_t pos_max)
{
    if (pos_max > 0)
        track_counts = (float)pos_max;
    integral = 0.0f;
}

void controller_reset(void)
{
    integral = 0.0f;
}

int8_t controller_update(int32_t ref, int32_t pos)
{
    float e = (float)(ref - pos) * 100.0f / track_counts;   // percent of track
    float u = KP * e + KI * integral;

    // Anti-windup: only integrate while the output is not saturated
    if (u > U_MAX)
        u = U_MAX;
    else if (u < -U_MAX)
        u = -U_MAX;
    else
        integral += e * T;

    return (int8_t)u;
}
