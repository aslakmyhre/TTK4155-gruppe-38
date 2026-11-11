#include "sam.h"
#include "encoder.h"

void encoder_init(void) {
    //enable clock for TC2 channel 0 (TC6)
    PMC->PMC_PCER1 = 1u << (ID_TC6-32);

    //give encoder pins PC25 and PC26 to the timer (peripheral B)
    PIOC->PIO_PDR = PIO_PC25 | PIO_PC26;
    PIOC->PIO_ABSR |= PIO_PC25 | PIO_PC26;

    //quadrature decoder mode, counts position from both encoder signals
    TC2->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_XC0 | TC_CMR_ETRGEDG_RISING | TC_CMR_ABETRG;
    TC2->TC_BMR = TC_BMR_QDEN | TC_BMR_POSEN | TC_BMR_EDGPHA;
    //enable and start counter
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

//returns position in encoder counts
int32_t encoder_read(void){
    return (int32_t)TC2->TC_CHANNEL[0].TC_CV;
}

//set position to 0
void encoder_reset(void) {
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
}