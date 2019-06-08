/* Copyright (C) Robert Reid, David Snowdon, 2009 <scandal@snowdon.id.au> */ 

/* 
 * This file is part of the UNSWMPPTNG firmware.
 * 
 * The UNSWMPPTNG firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The UNSWMPPTNG firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the UNSWMPPTNG firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Robert Reid
 * 07/09/2003
 * 
 * David Snowdon
 * 07/10/2004
 * 
 * ADC driver and interrupt handler for MSP430
 * 
 */


#include <msp430x14x.h>
#include <signal.h>
#include <stdmsp430.h>
#include <string.h>

#include "scandal_types.h"
#include "scandal_adc.h"
#include "control.h"

/* Set based on order of filter required */
/* Currently using 1st order IIR filter; otherwise 
 * extend this number if converted to using FIR filter later
 */

#define ADC_NUM_CHANNELS           10
#define ADC_FILTER_ORDER           1

/* Digital filtering scheme */ 
/* Note that this does not implement higher-order filters */ 
#define DIGITAL_FILTER(samples, new_sample){\
    samples >>= 1;			    \
    samples += (new_sample);		    \
}

/* ADC code */ 
static volatile u32 samples[ADC_NUM_CHANNELS]; 

void init_adc(void) {
	/* Enable all ADC channels - this should be done elsewhere*/
	// P6SEL = 0xff;

	/* Turn on 2.5V reference, enable ADC */
	/* Sample hold timer setting ?, Mulitple sample/conversion */
	ADC12CTL0 = ADC12ON | SHT0_9 | SHT1_9 | REFON | REF2_5V | MSC;  
  
	/* Repeated, sequence mode; Use sampling timer, SMCLK */
	ADC12CTL1 = SHP | CONSEQ_3 | ADC12SSEL_3; 

	/* Monitor channels 0,1,2,3,4,5,6,7,
	 * 	10 -- Temp,
	 * 	11 -- (Vcc-Vss)/2
	 *
	 * using 2.5V reference for all
	 */

	ADC12MCTL0 = SREF_1 | INCH_0;
	ADC12MCTL1 = SREF_1 | INCH_1;
	ADC12MCTL2 = SREF_1 | INCH_2;
	ADC12MCTL3 = SREF_1 | INCH_3;
	ADC12MCTL4 = SREF_1 | INCH_4;
	ADC12MCTL5 = SREF_1 | INCH_5;
	ADC12MCTL6 = SREF_1 | INCH_6;
	ADC12MCTL7 = SREF_1 | INCH_7;
	ADC12MCTL8 = SREF_1 | INCH_10;
	ADC12MCTL9 = SREF_1 | INCH_11;

	/* Unused ADC registers: */
	/*
	ADC12MCTL10 = SREF_1 | INCH_;
	ADC12MCTL11 = SREF_1 | INCH_;
	ADC12MCTL12 = SREF_1 | INCH_;
	ADC12MCTL13 = SREF_1 | INCH_;
	ADC12MCTL14 = SREF_1 | INCH_;
	ADC12MCTL15 = SREF_1 | INCH_;
	*/
	
	/* Enable interrupt for ADC12MCTLx (ADC12IFG.x), (1<<x) */
	ADC12IE = (1 << 9);


	/* Clear sample arrays */
	memset((u32*)samples, 0, sizeof(samples[0])
				* ADC_NUM_CHANNELS * ADC_FILTER_ORDER);

	/* Enable conversions */
	ADC12CTL0 |= ENC | ADC12SC;
}


/* ADC Interrupt */
interrupt (ADC_VECTOR) ADC12ISR(void) {
#if ADC_FILTER_ORDER != 1
  #error "Filter of order higher than 1 haven't been implemented"
	/* Need to shift all of the samples down one here */ 
#endif

    	DIGITAL_FILTER(samples[0], ADC12MEM0);
        DIGITAL_FILTER(samples[1], ADC12MEM1);
        DIGITAL_FILTER(samples[2], ADC12MEM2);
        DIGITAL_FILTER(samples[3], ADC12MEM3);
        DIGITAL_FILTER(samples[4], ADC12MEM4);
        DIGITAL_FILTER(samples[5], ADC12MEM5);
        DIGITAL_FILTER(samples[6], ADC12MEM6);
        DIGITAL_FILTER(samples[7], ADC12MEM7);
        DIGITAL_FILTER(samples[8], ADC12MEM8);
        DIGITAL_FILTER(samples[9], ADC12MEM9);

	/* This is the control loop for the MPPTs - 
	   Should be removed if you're not actually running an MPPT */
	control_loop();
}

uint16_t read_adc_value(u08 channel){
  return samples[channel];
}

u16 sample_adc(u08 channel){
  u32 sample; 

  /* Disable the ADC interrupt */ 
  ADC12IE &= ~(1 << 9);
  
  /* Short pause to make sure its off */ 
  {volatile int i; 
    for(i=15; i>0; i--)
      ; 
  }

  sample = read_adc_value(channel); 

  /* Turn the interrupt back on again */ 
  ADC12IE |= (1<<9); 

  /* Return the most recent value of the ADC - Unscaled */
  return(sample); 
}
