#include "sam.h"
#include "encoder.h"

void encoder_init(void) {
    PMC->PMC_PCER1 = 1u << (ID_TC6-32);

    PIOC->PIO_PDR = PIO_PC25 | PIO_PC26;
    PIOC->PIO_ABSR |= PIO_PC25 | PIO_PC26;

    TC2->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_XC0 | TC_CMR_ETRGEDG_RISING | TC_CMR_ABETRG;
    TC2->TC_BMR = TC_BMR_QDEN | TC_BMR_POSEN | TC_BMR_EDGPHA;
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

int32_t encoder_read(void){
    return (int32_t)TC2->TC_CHANNEL[0].TC_CV;
}

void encoder_reset(void) {
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
}