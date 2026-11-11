#include "controller.h"
#include "motor.h"

//controller gains and sample time in seconds
#define KP 2.0f
#define KI 1.0f
#define T (CONTROLLER_PERIOD_MS/1000.0f)
#define U_MAX ((float)MOTOR_MAX_PCT)   // saturate where the motor does, or anti-windup never kicks in

//track length in encoder counts
static float track_counts = 1.0f;
//sum of error over time
static float integral = 0.0f;
//values from last update, for debug
static ControllerState state = {0};

//save track length, reset integral
void controller_init(int32_t pos_max) {
    if (pos_max>0)
        track_counts = (float)pos_max;
    integral = 0.0f;
}

//clear integral and saved state
void controller_reset(void){
    integral = 0.0f;
    state = (ControllerState){0};
}

//returns values from last update
ControllerState controller_state(void){
    return state;
}

//PI controller, returns motor speed in %
int8_t controller_update(int32_t ref, int32_t pos) {
    //error in % of track length
    float e = (float)(ref-pos)*100.0f/track_counts;
    float p = KP*e;
    float i = KI*integral;
    float u = p+i;

    //anti windup
    if (u>U_MAX)
        u=U_MAX;
    else if (u<-U_MAX)
        u=-U_MAX;
    else 
        integral+=e*T;

    //save values for debug print
    state = (ControllerState){ .ref = ref, .pos = pos, .e = e, .p = p, .i = i, .u = u };

    return (int8_t)u;
}