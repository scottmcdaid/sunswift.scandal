#include <scandal/system.h>
#include <msp430.h>

void system_reset(void) {
	/* Reset the node here */
	/* Write an invalid password to the WDT */
	WDTCTL = ~WDTPW;
}
