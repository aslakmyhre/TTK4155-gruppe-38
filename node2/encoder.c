#include "sam.h"
#include "encoder.h"

void encoder_init(void)
{
    // TC6 (ID 33): PCER1
    PMC->PMC_PCER1 = (1u << (ID_TC6 - 32));

    // PIO releases PC25/PC26, peripheral B (TIOA6/TIOB6) occupies them
    PIOC->PIO_PDR   = PIO_PC25 | PIO_PC26;
    PIOC->PIO_ABSR |= PIO_PC25 | PIO_PC26;

    // Capture mode, clocked by XC0 (the decoder output), as the datasheet requires (36.6.14.4)
    TC2->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_XC0
                              | TC_CMR_ETRGEDG_RISING
                              | TC_CMR_ABETRG;

    // Quadrature decoder, position mode, count edges on both phases (x4)
    TC2->TC_BMR = TC_BMR_QDEN | TC_BMR_POSEN | TC_BMR_EDGPHA;

    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

int32_t encoder_read(void)
{
    return (int32_t)TC2->TC_CHANNEL[0].TC_CV;
}

void encoder_reset(void)
{
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
}
