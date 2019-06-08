/****************************************************************************
 *   $Id:: gpio.c 4785 2010-09-03 22:39:27Z nxp21346                        $
 *   Project: NXP LPC11xx GPIO example
 *
 *   Description:
 *     This file contains GPIO code example which include GPIO 
 *     initialization, GPIO interrupt handler, and related APIs for 
 *     GPIO access.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
#include <project/driver_config.h>
#include <arch/gpmsp430.h>

#define NUM_PORTS 4
#define MAX_NUM_HANDLERS 12

/* predefinitions */
void GPIO_IntClear(uint32_t portNum, uint32_t bitPosi);

static GPIO_InterruptHandler interrupt_handlers[NUM_PORTS][MAX_NUM_HANDLERS];

void PIOINT0_IRQHandler(void) {
	int i;
	for(i = 0; i < MAX_NUM_HANDLERS; i++) {
		if(GPIO_IntStatus(PORT0, i) && interrupt_handlers[PORT0][i] != 0) {
			GPIO_InterruptHandler handler = interrupt_handlers[PORT0][i];
			handler();
		}
	}
}

void PIOINT1_IRQHandler(void) {
	int i;
	for(i = 0; i < MAX_NUM_HANDLERS; i++) {
		if(GPIO_IntStatus(PORT1, i) && interrupt_handlers[PORT1][i] != 0) {
			GPIO_InterruptHandler handler = interrupt_handlers[PORT1][i];
			handler();
		}
	}
}

void PIOINT2_IRQHandler(void) {
	int i;
	for(i = 0; i < MAX_NUM_HANDLERS; i++) {
		if(GPIO_IntStatus(PORT2, i) && interrupt_handlers[PORT2][i] != 0) {
			GPIO_InterruptHandler handler = interrupt_handlers[PORT2][i];
			handler();
		}
	}
}

void PIOINT3_IRQHandler(void) {
	int i;

	for(i = 0; i < MAX_NUM_HANDLERS; i++) {
		if(GPIO_IntStatus(PORT3, i) && interrupt_handlers[PORT3][i] != 0) {
			GPIO_InterruptHandler handler = interrupt_handlers[PORT3][i];
			handler();
		}
	}
}

void GPIO_RegisterInterruptHandler(uint32_t port, uint32_t bit, uint32_t sense, 
		uint32_t single, uint32_t event, GPIO_InterruptHandler handler) {
	if (port < NUM_PORTS /* we only have 4 ports available */
		&& bit < MAX_NUM_HANDLERS /* there are a max of 12 bits on the ports */) {
		interrupt_handlers[port][bit] = handler;
		GPIO_SetInterrupt(port, bit, sense, single, event);
		GPIO_IntEnable(port, bit);
	}
}

void GPIO_SetFunction(uint32_t port, uint32_t bit, uint32_t func, uint32_t mode) {

	/* for the specified port and pin, first set the function bits to 0 and then
	 * to the function we actually want */

	/* this chip is retarded. every IOCON register has different meanings for the
	 * bits. I'm starting to hate this chip */

	switch (port) {
	 case 0:
		switch (bit) {
		 case 0:
			/* this is the reset pin, we can't use it */
			return;
			LPC_IOCON->RESET_PIO0_0 &= ~0x7F;
			LPC_IOCON->RESET_PIO0_0 |= (func+1);
			LPC_IOCON->RESET_PIO0_0 |= (mode << 3);
			break;
		 case 1:
			LPC_IOCON->PIO0_1 &= ~0x7F;
			LPC_IOCON->PIO0_1 |= func;
			LPC_IOCON->PIO0_1 |= (mode << 3);
			break;
		 case 2:
			LPC_IOCON->PIO0_2 &= ~0x7F;
			LPC_IOCON->PIO0_2 |= func;
			LPC_IOCON->PIO0_2 |= (mode << 3);
			break;
		 case 3:
			LPC_IOCON->PIO0_3 &= ~0x7F;
			LPC_IOCON->PIO0_3 |= func;
			LPC_IOCON->PIO0_3 |= (mode << 3);
			break;
		 case 4:
			LPC_IOCON->PIO0_4 &= ~0x7F;
			LPC_IOCON->PIO0_4 |= func;
			LPC_IOCON->PIO0_4 |= (mode << 3);
			break;
		 case 5:
			LPC_IOCON->PIO0_5 &= ~0x7F;
			LPC_IOCON->PIO0_5 |= func;
			LPC_IOCON->PIO0_5 |= (mode << 3);
			break;
		 case 6:
			LPC_IOCON->PIO0_6 &= ~0x7F;
			LPC_IOCON->PIO0_6 |= func;
			LPC_IOCON->PIO0_6 |= (mode << 3);
			break;
		 case 7:
			LPC_IOCON->PIO0_7 &= ~0x7F;
			LPC_IOCON->PIO0_7 |= func;
			LPC_IOCON->PIO0_7 |= (mode << 3);
			break;
		 case 8:
			LPC_IOCON->PIO0_8 &= ~0x7F;
			LPC_IOCON->PIO0_8 |= func;
			LPC_IOCON->PIO0_8 |= (mode << 3);
			break;
		 case 9:
			/* this is TDO, it's used for programming */
			LPC_IOCON->PIO0_9 &= ~0x7F;
			LPC_IOCON->PIO0_9 |= func;
			LPC_IOCON->PIO0_9 |= (mode << 3);
			break;
		 case 10:
			LPC_IOCON->SWCLK_PIO0_10 &= ~0x7F;
			LPC_IOCON->SWCLK_PIO0_10 |= (func+1);
			LPC_IOCON->SWCLK_PIO0_10 |= (mode << 3);
			/* this is TCLK, it's used for programming */
			break;
		 case 11:
			LPC_IOCON->R_PIO0_11 &= ~0x7F;
			LPC_IOCON->R_PIO0_11 |= (func+1);
			LPC_IOCON->R_PIO0_11 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->R_PIO0_11 &= ~(0x80);
			break;
		}
		break;

	 case 1:
		switch (bit) {
		 case 0:
			LPC_IOCON->R_PIO1_0 &= ~0x7F;
			LPC_IOCON->R_PIO1_0 |= (func+1);
			LPC_IOCON->R_PIO1_0 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->R_PIO1_0 &= ~(0x80);
			break;
		 case 1:
			LPC_IOCON->R_PIO1_1 &= ~0x7F;
			LPC_IOCON->R_PIO1_1 |= (func+1);
			LPC_IOCON->R_PIO1_1 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->R_PIO1_1 &= ~(0x80);
			break;
		 case 2:
			LPC_IOCON->R_PIO1_2 &= ~0x7F;
			LPC_IOCON->R_PIO1_2 |= (func+1);
			LPC_IOCON->R_PIO1_2 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->R_PIO1_2 &= ~(0x80);
			break;
		 case 3:
			/* this is TMS, it's used for programming */
			return;
		 case 4:
			LPC_IOCON->PIO1_4 &= ~0x7F;
			LPC_IOCON->PIO1_4 |= func;
			LPC_IOCON->PIO1_4 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->PIO1_4 &= ~(0x80);
			break;
		 case 5:
			LPC_IOCON->PIO1_5 &= ~0x7F;
			LPC_IOCON->PIO1_5 |= func;
			LPC_IOCON->PIO1_5 |= (mode << 3);
			break;
		 case 6:
			LPC_IOCON->PIO1_6 &= ~0x7F;
			LPC_IOCON->PIO1_6 |= func;
			LPC_IOCON->PIO1_6 |= (mode << 3);
			break;
		 case 7:
			LPC_IOCON->PIO1_7 &= ~0x7F;
			LPC_IOCON->PIO1_7 |= func;
			LPC_IOCON->PIO1_7 |= (mode << 3);
			break;
		 case 8:
			LPC_IOCON->PIO1_8 &= ~0x7F;
			LPC_IOCON->PIO1_8 |= func;
			LPC_IOCON->PIO1_8 |= (mode << 3);
			break;
		 case 9:
			LPC_IOCON->PIO1_9 &= ~0x7F;
			LPC_IOCON->PIO1_9 |= func;
			LPC_IOCON->PIO1_9 |= (mode << 3);
			break;
		 case 10:
			LPC_IOCON->PIO1_10 &= ~0x7F;
			LPC_IOCON->PIO1_10 |= func;
			LPC_IOCON->PIO1_10 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->PIO1_10 &= ~(0x80);
			break;
		 case 11:
			LPC_IOCON->PIO1_11 &= ~0x7F;
			LPC_IOCON->PIO1_11 |= func;
			LPC_IOCON->PIO1_11 |= (mode << 3);
			if (func == GPIO_FUNC1) /* ADC Mode, set to analog */
				LPC_IOCON->PIO1_11 &= ~(0x80);
			break;
		}
		break;

	 case 2:
		switch (bit) {
		 case 0:
			LPC_IOCON->PIO2_0 &= ~0x7F;
			LPC_IOCON->PIO2_0 |= func;
			LPC_IOCON->PIO2_0 |= (mode << 3);
			break;
		 case 1:
			LPC_IOCON->PIO2_1 &= ~0x7F;
			LPC_IOCON->PIO2_1 |= func;
			LPC_IOCON->PIO2_1 |= (mode << 3);
			break;
		 case 2:
			LPC_IOCON->PIO2_2 &= ~0x7F;
			LPC_IOCON->PIO2_2 |= func;
			LPC_IOCON->PIO2_2 |= (mode << 3);
			break;
		 case 3:
			LPC_IOCON->PIO2_3 &= ~0x7F;
			LPC_IOCON->PIO2_3 |= func;
			LPC_IOCON->PIO2_3 |= (mode << 3);
			break;
		 case 4:
			LPC_IOCON->PIO2_4 &= ~0x7F;
			LPC_IOCON->PIO2_4 |= func;
			LPC_IOCON->PIO2_4 |= (mode << 3);
			break;
		 case 5:
			LPC_IOCON->PIO2_5 &= ~0x7F;
			LPC_IOCON->PIO2_5 |= func;
			LPC_IOCON->PIO2_5 |= (mode << 3);
			break;
		 case 6:
			LPC_IOCON->PIO2_6 &= ~0x7F;
			LPC_IOCON->PIO2_6 |= func;
			LPC_IOCON->PIO2_6 |= (mode << 3);
			break;
		 case 7:
			LPC_IOCON->PIO2_7 &= ~0x7F;
			LPC_IOCON->PIO2_7 |= func;
			LPC_IOCON->PIO2_7 |= (mode << 3);
			break;
		 case 8:
			LPC_IOCON->PIO2_8 &= ~0x7F;
			LPC_IOCON->PIO2_8 |= func;
			LPC_IOCON->PIO2_8 |= (mode << 3);
			break;
		 case 9:
			LPC_IOCON->PIO2_9 &= ~0x7F;
			LPC_IOCON->PIO2_9 |= func;
			LPC_IOCON->PIO2_9 |= (mode << 3);
			break;
		 case 10:
			LPC_IOCON->PIO2_10 &= ~0x7F;
			LPC_IOCON->PIO2_10 |= func;
			LPC_IOCON->PIO2_10 |= (mode << 3);
			break;
		 case 11:
			LPC_IOCON->PIO2_11 &= ~0x7F;
			LPC_IOCON->PIO2_11 |= func;
			LPC_IOCON->PIO2_11 |= (mode << 3);
			break;
		}
		break;

	 case 3:
		switch (bit) {
		 case 0:
			LPC_IOCON->PIO3_0 &= ~0x7F;
			LPC_IOCON->PIO3_0 |= func;
			LPC_IOCON->PIO3_0 |= (mode << 3);
			break;
		 case 1:
			LPC_IOCON->PIO3_1 &= ~0x7F;
			LPC_IOCON->PIO3_1 |= func;
			LPC_IOCON->PIO3_1 |= (mode << 3);
			break;
		 case 2:
			LPC_IOCON->PIO3_2 &= ~0x7F;
			LPC_IOCON->PIO3_2 |= func;
			LPC_IOCON->PIO3_2 |= (mode << 3);
			break;
		 case 3:
			LPC_IOCON->PIO3_3 &= ~0x7F;
			LPC_IOCON->PIO3_3 |= func;
			LPC_IOCON->PIO3_3 |= (mode << 3);
			break;
		 case 4:
			LPC_IOCON->PIO3_4 &= ~0x7F;
			LPC_IOCON->PIO3_4 |= func;
			LPC_IOCON->PIO3_4 |= (mode << 3);
			break;
		 case 5:
			LPC_IOCON->PIO3_5 &= ~0x7F;
			LPC_IOCON->PIO3_5 |= func;
			LPC_IOCON->PIO3_5 |= (mode << 3);
			break;
		}
		break;
	}
}

void GPIO_Init( void ) {
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

#ifdef __JTAG_DISABLED  
	LPC_IOCON->R_PIO1_1  |= 0x01;
#endif

	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
}

/*****************************************************************************
** Function name:		GPIOSetInterrupt
**
** Descriptions:		Set interrupt sense, event, etc.
**						edge or level, 0 is edge, 1 is level
**						single or double edge, 0 is single, 1 is double 
**						active high or low, etc.
**
** parameters:			port num, bit position, sense, single/doube, polarity
** Returned value:		None
** 
*****************************************************************************/
void GPIO_SetInterrupt( uint32_t portNum, uint32_t bitPosi, uint32_t sense,
			uint32_t single, uint32_t event ) {

	switch ( portNum ) {
	 case PORT0:
		if ( sense == 0 ) {
			LPC_GPIO0->IS &= ~(0x1<<bitPosi);
			/* single or double only applies when sense is 0(edge trigger). */
			if ( single == 0 )
				LPC_GPIO0->IBE &= ~(0x1<<bitPosi);
			else
				LPC_GPIO0->IBE |= (0x1<<bitPosi);

		} else {
			LPC_GPIO0->IS |= (0x1<<bitPosi);
		}
		
		if ( event == 0 )
			LPC_GPIO0->IEV &= ~(0x1<<bitPosi);
		else
			LPC_GPIO0->IEV |= (0x1<<bitPosi);		
		
		break;

	 case PORT1:
		if ( sense == 0 ) {
			LPC_GPIO1->IS &= ~(0x1<<bitPosi);
			/* single or double only applies when sense is 0(edge trigger). */
			if ( single == 0 )
				LPC_GPIO1->IBE &= ~(0x1<<bitPosi);
			else
				LPC_GPIO1->IBE |= (0x1<<bitPosi);
		} else {
			LPC_GPIO1->IS |= (0x1<<bitPosi);
		}
			
		if ( event == 0 )
			LPC_GPIO1->IEV &= ~(0x1<<bitPosi);
		else
			LPC_GPIO1->IEV |= (0x1<<bitPosi);
		
		break;

	 case PORT2:
		if ( sense == 0 ) {
			LPC_GPIO2->IS &= ~(0x1<<bitPosi);
			/* single or double only applies when sense is 0(edge trigger). */
			if ( single == 0 )
				LPC_GPIO2->IBE &= ~(0x1<<bitPosi);
			else
				LPC_GPIO2->IBE |= (0x1<<bitPosi);
		} else {
			LPC_GPIO2->IS |= (0x1<<bitPosi);
		}

		if ( event == 0 )
			LPC_GPIO2->IEV &= ~(0x1<<bitPosi);
		else
			LPC_GPIO2->IEV |= (0x1<<bitPosi);  
		
		break;

	 case PORT3:
		if ( sense == 0 ) {
			LPC_GPIO3->IS &= ~(0x1<<bitPosi);
			/* single or double only applies when sense is 0(edge trigger). */
			if ( single == 0 )
				LPC_GPIO3->IBE &= ~(0x1<<bitPosi);
			else
				LPC_GPIO3->IBE |= (0x1<<bitPosi);
		} else {
			LPC_GPIO3->IS |= (0x1<<bitPosi);
		}

		if ( event == 0 )
			LPC_GPIO3->IEV &= ~(0x1<<bitPosi);
		else
			LPC_GPIO3->IEV |= (0x1<<bitPosi);
				
		break;
	}
}

void GPIO_IntEnable( uint32_t portNum, uint32_t bitPosi ) {
	switch ( portNum ) {
	 case PORT0:
		LPC_GPIO0->IE |= (0x1<<bitPosi);
		break;
	 case PORT1:
		LPC_GPIO1->IE |= (0x1<<bitPosi);
		break;
	 case PORT2:
		LPC_GPIO2->IE |= (0x1<<bitPosi);
		break;
	 case PORT3:
		LPC_GPIO3->IE |= (0x1<<bitPosi);
		break;
	}
}

void GPIO_IntDisable( uint32_t portNum, uint32_t bitPosi ) {
	switch ( portNum ) {
	 case PORT0:
		LPC_GPIO0->IE &= ~(0x1<<bitPosi);
		break;
	 case PORT1:
		LPC_GPIO1->IE &= ~(0x1<<bitPosi);
		break;
	 case PORT2:
		LPC_GPIO2->IE &= ~(0x1<<bitPosi);
		break;
	 case PORT3:
		LPC_GPIO3->IE &= ~(0x1<<bitPosi);
		break;
  }
  return;
}

uint32_t GPIO_IntStatus( uint32_t portNum, uint32_t bitPosi ) {
	uint32_t regVal = 0;

	switch ( portNum ) {
	 case PORT0:
		if ( LPC_GPIO0->MIS & (0x1<<bitPosi) )
			regVal = 1;
		break;
	 case PORT1:
		if ( LPC_GPIO1->MIS & (0x1<<bitPosi) )
			regVal = 1;
		break;
	case PORT2:
		if ( LPC_GPIO2->MIS & (0x1<<bitPosi) )
			regVal = 1;
		break;
	case PORT3:
		if ( LPC_GPIO3->MIS & (0x1<<bitPosi) )
			regVal = 1;
		break;
	}

	return regVal;
}

void GPIO_IntClear( uint32_t portNum, uint32_t bitPosi ) {
	switch ( portNum ) {
	 case PORT0:
		LPC_GPIO0->IC |= (0x1<<bitPosi);
		break;
	 case PORT1:
		LPC_GPIO1->IC |= (0x1<<bitPosi);
		break;
	 case PORT2:
		LPC_GPIO2->IC |= (0x1<<bitPosi);
		break;
	case PORT3:
		LPC_GPIO3->IC |= (0x1<<bitPosi);
		break;
	}
}

void GPIO_SetValue( uint32_t portNum, uint32_t bitPosi, uint32_t bitVal ) {
	LPC_GPIO[portNum]->MASKED_ACCESS[(1<<bitPosi)] = (bitVal<<bitPosi);
}

uint32_t GPIO_GetValue( uint32_t portNum, uint32_t bitPosi) {
	return LPC_GPIO[portNum]->MASKED_ACCESS[(1<<bitPosi)] >> bitPosi;
}

void GPIO_ToggleValue(uint32_t portNum, uint32_t bitPosi) {
	LPC_GPIO[portNum]->MASKED_ACCESS[(1<<bitPosi)] ^= (1<<bitPosi);
}

/* portNum = Port number of the pin you want
   bitPosi = Bit number of the pin you want
   Dir = Input (0 or INPUT), Output (1 or OUTPUT)*/
void GPIO_SetDir( uint32_t portNum, uint32_t bitPosi, uint32_t dir ) {
	GPIO_SetFunction(portNum, bitPosi, 0, 0x4);
	if(dir == OUTPUT){
		LPC_GPIO[portNum]->DIR |= 1<<bitPosi;
	}else if(dir == INPUT){
		LPC_GPIO[portNum]->DIR &= ~(1<<bitPosi);
	}
}
