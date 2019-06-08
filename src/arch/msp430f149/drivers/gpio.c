//scott #include <arch/gpmsp430.h>
//#include <msp430x14x.h>
#include <msp430.h>

typedef unsigned long uint32_t;

void GPIO_Init( void ) {
	/* Nothing to do here for msp430 */
}

void GPIO_SetInterrupt( uint32_t portNum, uint32_t bitPosi, uint32_t sense,
						uint32_t single, uint32_t event ) {
	/* Not implemented */
}

void GPIO_IntEnable( uint32_t portNum, uint32_t bitPosi ) {
	/* Not implemented */
}

void GPIO_IntDisable( uint32_t portNum, uint32_t bitPosi ) {
	/* Not implemented */
}

uint32_t GPIO_IntStatus( uint32_t portNum, uint32_t bitPosi ) {
	/* Not implemented */
	return 0;
}

void GPIO_IntClear( uint32_t portNum, uint32_t bitPosi ) {
	/* Not implemented */
}

void GPIO_SetValue( uint32_t portNum, uint32_t bitPosi, uint32_t bitVal ) {
	if(bitVal) {
		switch(portNum) {
		 case 1:
			P1OUT |= (1<<bitPosi); break;
		 case 2:
			P2OUT |= (1<<bitPosi); break;
		 case 3:
			P3OUT |= (1<<bitPosi); break;
		 case 4:
			P4OUT |= (1<<bitPosi); break;
		 case 5:
			P5OUT |= (1<<bitPosi); break;
		 case 6:
			P6OUT |= (1<<bitPosi); break;
		}
	} else {
		switch(portNum) {
		 case 1:
			P1OUT &= ~(1<<bitPosi); break;
		 case 2:
			P2OUT &= ~(1<<bitPosi); break;
		 case 3:
			P3OUT &= ~(1<<bitPosi); break;
		 case 4:
			P4OUT &= ~(1<<bitPosi); break;
		 case 5:
			P5OUT &= ~(1<<bitPosi); break;
		 case 6:
			P6OUT &= ~(1<<bitPosi); break;
		}
	}
}

uint32_t GPIO_GetValue( uint32_t portNum, uint32_t bitPosi) {
	switch(portNum) {
	 case 1:
		return ((P1OUT & (1<<bitPosi)) >>bitPosi); break;
	 case 2:
		return ((P2OUT & (1<<bitPosi)) >>bitPosi); break;
	 case 3:
		return ((P3OUT & (1<<bitPosi)) >>bitPosi); break;
	 case 4:
		return ((P4OUT & (1<<bitPosi)) >>bitPosi); break;
	 case 5:
		return ((P5OUT & (1<<bitPosi)) >>bitPosi); break;
	 case 6:
		return ((P6OUT & (1<<bitPosi)) >>bitPosi); break;
	 default:
		return 0; // probably bad, we don't let the user know that they're out of ports.
	}
}

void GPIO_ToggleValue(uint32_t portNum, uint32_t bitPosi) {
	switch(portNum) {
	 case 1:
		P1OUT ^= (1<<bitPosi); break;
	 case 2:
		P2OUT ^= (1<<bitPosi); break;
	 case 3:
		P3OUT ^= (1<<bitPosi); break;
	 case 4:
		P4OUT ^= (1<<bitPosi); break;
	 case 5:
		P5OUT ^= (1<<bitPosi); break;
	 case 6:
		P6OUT ^= (1<<bitPosi); break;
	}
}

void GPIO_SetDir( uint32_t portNum, uint32_t bitPosi, uint32_t dir ) {
	if(dir) {
		switch(portNum) {
		 case 1:
			P1DIR |= (1<<bitPosi); break;
		 case 2:
			P2DIR |= (1<<bitPosi); break;
		 case 3:
			P3DIR |= (1<<bitPosi); break;
		 case 4:
			P4DIR |= (1<<bitPosi); break;
		 case 5:
			P5DIR |= (1<<bitPosi); break;
		 case 6:
			P6DIR |= (1<<bitPosi); break;
		}
	} else {
		switch(portNum) {
		 case 1:
			P1DIR &= ~(1<<bitPosi); break;
		 case 2:
			P2DIR &= ~(1<<bitPosi); break;
		 case 3:
			P3DIR &= ~(1<<bitPosi); break;
		 case 4:
			P4DIR &= ~(1<<bitPosi); break;
		 case 5:
			P5DIR &= ~(1<<bitPosi); break;
		 case 6:
			P6DIR &= ~(1<<bitPosi); break;
		}
	}
}
