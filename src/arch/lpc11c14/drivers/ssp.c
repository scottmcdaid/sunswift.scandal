/****************************************************************************
 *   $Id:: ssp.c 4785 2010-09-03 22:39:27Z nxp21346                         $
 *   Project: NXP LPC11xx SSP example
 *
 *   Description:
 *     This file contains SSP code which include SSP 
 *     initialization, SSP interrupt handler, and APIs for SSP
 *     reading.
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
#if CONFIG_ENABLE_DRIVER_SSP==1
#include <arch/gpmsp430.h>
#include <arch/ssp.h>

/* statistics of all the interrupts */
volatile uint32_t interruptRxStat0 = 0;
volatile uint32_t interruptOverRunStat0 = 0;
volatile uint32_t interruptRxTimeoutStat0 = 0;

volatile uint32_t interruptRxStat1 = 0;
volatile uint32_t interruptOverRunStat1 = 0;
volatile uint32_t interruptRxTimeoutStat1 = 0;

/*****************************************************************************
** Function name:		SSP0_IRQHandler
**
** Descriptions:		SSP port is used for SPI communication.
**						SSP interrupt handler
**						The algorithm is, if RXFIFO is at least half full, 
**						start receive until it's empty; if TXFIFO is at least
**						half empty, start transmit until it's full.
**						This will maximize the use of both FIFOs and performance.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP0_IRQHandler(void) 
{
  uint32_t regValue;

  regValue = LPC_SSP0->MIS;
  if ( regValue & SSPMIS_RORMIS )	/* Receive overrun interrupt */
  {
	interruptOverRunStat0++;
	LPC_SSP0->ICR = SSPICR_RORIC;	/* clear interrupt */
  }
  if ( regValue & SSPMIS_RTMIS )	/* Receive timeout interrupt */
  {
	interruptRxTimeoutStat0++;
	LPC_SSP0->ICR = SSPICR_RTIC;	/* clear interrupt */
  }

  /* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
  are shared as global variables. It may create some race condition that main
  and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
  in both main and ISR could prevent this kind of race condition */
  if ( regValue & SSPMIS_RXMIS )	/* Rx at least half full */
  {
	interruptRxStat0++;		/* receive until it's empty */		
  }
  return;
}

/*****************************************************************************
** Function name:		SSP1_IRQHandler
**
** Descriptions:		SSP port is used for SPI communication.
**						SSP interrupt handler
**						The algorithm is, if RXFIFO is at least half full, 
**						start receive until it's empty; if TXFIFO is at least
**						half empty, start transmit until it's full.
**						This will maximize the use of both FIFOs and performance.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP1_IRQHandler(void) 
{
  uint32_t regValue;

  regValue = LPC_SSP1->MIS;
  if ( regValue & SSPMIS_RORMIS )	/* Receive overrun interrupt */
  {
	interruptOverRunStat1++;
	LPC_SSP1->ICR = SSPICR_RORIC;	/* clear interrupt */
  }
  if ( regValue & SSPMIS_RTMIS )	/* Receive timeout interrupt */
  {
	interruptRxTimeoutStat1++;
	LPC_SSP1->ICR = SSPICR_RTIC;	/* clear interrupt */
  }

  /* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
  are shared as global variables. It may create some race condition that main
  and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
  in both main and ISR could prevent this kind of race condition */
  if ( regValue & SSPMIS_RXMIS )	/* Rx at least half full */
  {
	interruptRxStat1++;		/* receive until it's empty */		
  }
  return;
}

/*****************************************************************************
** Function name:		SSP_IOConfig
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			port #
** Returned value:		None
** 
*****************************************************************************/
void SSP_IOConfig( uint8_t portNum )
{
  if ( portNum == 0 )
{
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);
  LPC_SYSCON->SYSAHBCLKCTRL |= (0x1<<11);
  LPC_SYSCON->SSP0CLKDIV = 0x02;			/* Divided by 2 */
  LPC_IOCON->PIO0_8           &= ~0x07;	/*  SSP I/O config */
  LPC_IOCON->PIO0_8           |= 0x01;		/* SSP MISO */
  LPC_IOCON->PIO0_9           &= ~0x07;	
  LPC_IOCON->PIO0_9           |= 0x01;		/* SSP MOSI */
#ifdef USE_SCK_P0_10
	LPC_IOCON->SCK_LOC = 0x00;
	LPC_IOCON->SWCLK_PIO0_10 &= ~0x07;
	LPC_IOCON->SWCLK_PIO0_10 |= 0x02;		/* SSP CLK */
#else
#if 0
	/* On HummingBird/Candiru 1(HB1/CD1), SSP CLK can be routed to different 
	pins, other than JTAG TCK, it's either P2.11 func. 1 or P0.6 func. 2. */
  LPC_IOCON->SCK_LOC = 0x01;
	LPC_IOCON->PIO2_11 = 0x01;	/* P2.11 function 1 is SSP clock, need to 
								combined with IOCONSCKLOC register setting */
#else
	LPC_IOCON->SCK_LOC = 0x02;
	LPC_IOCON->PIO0_6 = 0x02;	/* P0.6 function 2 is SSP clock, need to 
								combined with IOCONSCKLOC register setting */
#endif
#endif	/* endif __JTAG_DISABLED */  

#if USE_CS
  LPC_IOCON->PIO0_2 &= ~0x07;	
  LPC_IOCON->PIO0_2 |= 0x01;		/* SSP SSEL */
#else
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

  LPC_IOCON->PIO0_2 &= ~0x07;		/* SSP SSEL is a GPIO pin */
  /* port0, bit 2 is set to GPIO output and high */
  GPIO_SetDir( PORT0, 2, 1 );
  GPIO_SetValue( PORT0, 2, 1 );
#endif
  }
  else		/* port number 1 */
  {
	LPC_SYSCON->PRESETCTRL |= (0x1<<2);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<18);
	LPC_SYSCON->SSP1CLKDIV = 0x02;			/* Divided by 2 */
	LPC_IOCON->PIO2_2 &= ~0x07;	/*  SSP I/O config */
	LPC_IOCON->PIO2_2 |= 0x02;		/* SSP MISO */
	LPC_IOCON->PIO2_3 &= ~0x07;
	LPC_IOCON->PIO2_3 |= 0x02;
	//LPC_IOCON->PIO2_3 &= ~0x73D;	
	//LPC_IOCON->PIO2_3 |= 0xC2;		/* SSP MOSI */
	LPC_IOCON->PIO2_1 &= ~0x07;
	LPC_IOCON->PIO2_1 |= 0x02;		/* SSP CLK */
 
#if USE_CS || USE_CS_1
	LPC_IOCON->PIO2_0 &= ~0x07;	
	LPC_IOCON->PIO2_0 |= 0x02;		/* SSP SSEL */
#else
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);
		
//Why is the below commented out and not the above?
	LPC_IOCON->PIO2_0 &= ~0x07;		/* SSP SSEL is a GPIO pin */
	/* port2, bit 0 is set to GPIO output and high */
	GPIO_SetDir( PORT2, 0, 1 );
	GPIO_SetValue( PORT2, 0, 1 );
	//LPC_IOCON->PIO2_0 |= 0x02;
#endif
  }
  return;		
}

/*****************************************************************************
** Function name:		SSP_Init
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			port #
** Returned value:		None
** 
** ORIGINAL CODE EXAMPLE
*****************************************************************************/
void SSP_Init( uint8_t portNum )
{
  uint8_t i, Dummy=Dummy;

  if ( portNum == 0 )
  {
  /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 15 */
	LPC_SSP0->CR0 = 0x0707;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
	LPC_SSP0->CPSR = 0x2;

  for ( i = 0; i < FIFOSIZE; i++ )
  {
	  Dummy = LPC_SSP0->DR;		/* clear the RxFIFO */
  }

  /* Enable the SSP Interrupt */
	NVIC_EnableIRQ(SSP0_IRQn);
	
  /* Device select as master, SSP Enabled */
#if LOOPBACK_MODE
	LPC_SSP0->CR1 = SSPCR1_LBM | SSPCR1_SSE;
#else
#if SSP_SLAVE
  /* Slave mode */
	if ( LPC_SSP0->CR1 & SSPCR1_SSE )
  {
	/* The slave bit can't be set until SSE bit is zero. */
	  LPC_SSP0->CR1 &= ~SSPCR1_SSE;
  }
	LPC_SSP0->CR1 = SSPCR1_MS;		/* Enable slave bit first */
	LPC_SSP0->CR1 |= SSPCR1_SSE;	/* Enable SSP */
#else
  /* Master mode */
	LPC_SSP0->CR1 = SSPCR1_SSE;
#endif
#endif
  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
	LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
  }
  else
  {
	/* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 15 */
	LPC_SSP1->CR0 = 0x0787; //currently set to clock data out on falling edge

	/* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
	LPC_SSP1->CPSR = 0x2;

	for ( i = 0; i < FIFOSIZE; i++ )
	{
	  Dummy = LPC_SSP1->DR;		/* clear the RxFIFO */
	}

	/* Enable the SSP Interrupt */
	NVIC_EnableIRQ(SSP1_IRQn);
	
	/* Device select as master, SSP Enabled */
#if LOOPBACK_MODE
	LPC_SSP1->CR1 = SSPCR1_LBM | SSPCR1_SSE;
#else
#if SSP_SLAVE
	/* Slave mode */
	if ( LPC_SSP1->CR1 & SSPCR1_SSE )
	{
	  /* The slave bit can't be set until SSE bit is zero. */
	  LPC_SSP1->CR1 &= ~SSPCR1_SSE;
	}
	LPC_SSP1->CR1 = SSPCR1_MS;		/* Enable slave bit first */
	LPC_SSP1->CR1 |= SSPCR1_SSE;	/* Enable SSP */
#else
	/* Master mode */
	LPC_SSP1->CR1 = SSPCR1_SSE;
#endif
#endif
	/* Set SSPINMS registers to enable interrupts */
	/* enable all error related interrupts */
	LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
  }
  return;
}

/*****************************************************************************

** Function name:		SSP0_Init
**
** Descriptions:		SSP0 port initialization routine
**				

** parameters:			initialisation variables 
** Returned value:		None
** 
*****************************************************************************/

void SSP0_Init(SSP_init_struct *initVars){
  
  uint8_t i;
  uint32_t temp; //scratch register
  
  temp = (initVars->DataSize & 0xF) << 0;
  temp |= ((initVars->FrameFormat & 0x3) << 4);
  temp |= ((initVars->ClockPolarity & 0x1) << 6);
  temp |= ((initVars->ClockPhase & 0x1) << 7);
  temp |= ((initVars->ClockRate & 0xFF) << 8); //SCR
  
  //Set SSP Control Register 0: SCR(15:8), CPHA(7), CPOL(6), FRF(5:4), DSS(3:0)
  LPC_SSP0->CR0 = ((initVars->ClockRate & 0xFF) << 8) | ((initVars->ClockPhase & 0x1) << 7) | ((initVars->ClockPolarity & 0x1) << 6) | ((initVars->FrameFormat & 0x3) << 4) | ((initVars->DataSize & 0xF) << 0);

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SSP0->CPSR = initVars->ClockPrescale;

  /* clear the RxFIFO */
  for ( i = 0; i < FIFOSIZE; i++ )
  {
    temp = LPC_SSP0->DR;		
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP0_IRQn);
	
  /* Device select as master, SSP Enabled */
  
  /*
    Previous initialisation for Loopback mode:
    LPC_SSP0->CR1 = SSPCR1_LBM | SSPCR1_SSE;

    Previous initialisation for Slave mode (Currently not implemented!)
   */
  
  if(initVars->Slave){
    /* Initialise in slave mode */
    
    if ( LPC_SSP0->CR1 & SSPCR1_SSE ){
	/* The slave bit can't be set until SSE bit is zero. */
      LPC_SSP0->CR1 &= ~SSPCR1_SSE;
    }
    
      LPC_SSP0->CR1 = SSPCR1_MS;	/* Enable slave bit first */
      LPC_SSP0->CR1 |= SSPCR1_SSE;	/* Enable SSP */
      
  }else{
    /* Initialise in master mode */
    LPC_SSP0->CR1 = SSPCR1_SSE;
  }

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;    

    return;
}


/*****************************************************************************
** Function name:		SSP1_Init
**
** Descriptions:		SSP1 port initialization routine
**				
** parameters:			initialisation variables 
** Returned value:		None
** 
*****************************************************************************/

void SSP1_Init(SSP_init_struct *initVars){
  
  uint8_t i;
  uint32_t temp; //scratch register
  
  temp = (initVars->DataSize & 0xF) << 0;
  temp |= ((initVars->FrameFormat & 0x3) << 4);
  temp |= ((initVars->ClockPolarity & 0x1) << 6);
  temp |= ((initVars->ClockPhase & 0x1) << 7);
  temp |= ((initVars->ClockRate & 0xFF) << 8); //SCR
  
  //Set SSP Control Register 0: SCR(15:8), CPHA(7), CPOL(6), FRF(5:4), DSS(3:0)
  LPC_SSP1->CR0 = ((initVars->ClockRate & 0xFF) << 8) | ((initVars->ClockPhase & 0x1) << 7) | ((initVars->ClockPolarity & 0x1) << 6) | ((initVars->FrameFormat & 0x3) << 4) | ((initVars->DataSize & 0xF) << 0);

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SSP1->CPSR = initVars->ClockPrescale;

  /* clear the RxFIFO */
  for ( i = 0; i < FIFOSIZE; i++ )
  {
    temp = LPC_SSP1->DR;        
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP1_IRQn);
    
  /* Device select as master, SSP Enabled */
  
  /*
    Previous initialisation for Loopback mode:
    LPC_SSP1->CR1 = SSPCR1_LBM | SSPCR1_SSE;

    Previous initialisation for Slave mode (Currently not implemented!)
   */
  
  if(initVars->Slave){
    /* Initialise in slave mode */
    
    if ( LPC_SSP1->CR1 & SSPCR1_SSE ){
    /* The slave bit can't be set until SSE bit is zero. */
      LPC_SSP1->CR1 &= ~SSPCR1_SSE;
    }
    
      LPC_SSP1->CR1 = SSPCR1_MS;    /* Enable slave bit first */
      LPC_SSP1->CR1 |= SSPCR1_SSE;  /* Enable SSP */
      
  }else{
    /* Initialise in master mode */
    LPC_SSP1->CR1 = SSPCR1_SSE;
  }

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;    

    return;
}

/*****************************************************************************
** Function name:		SSP_new_Init
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			initialisation variables, 
**                      port (LPC_SSP0 / LPC_SSP1)
** Returned value:		None
** 
*****************************************************************************/

void SSP_new_Init(SSP_init_struct *initVars, LPC_SSP_TypeDef *port){
  
  uint8_t i;
  uint32_t temp; //scratch register
  
  temp = (initVars->DataSize & 0xF) << 0;
  temp |= ((initVars->FrameFormat & 0x3) << 4);
  temp |= ((initVars->ClockPolarity & 0x1) << 6);
  temp |= ((initVars->ClockPhase & 0x1) << 7);
  temp |= ((initVars->ClockRate & 0xFF) << 8); //SCR
  
  //Set SSP Control Register 0: SCR(15:8), CPHA(7), CPOL(6), FRF(5:4), DSS(3:0)
  port->CR0 = temp;
  
  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  port->CPSR = initVars->ClockPrescale;

  /* clear the RxFIFO */
  
  for ( i = 0; i < FIFOSIZE; i++ )
  {
    temp = port->DR;        
  }
  
  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP1_IRQn);
    
  /* Device select as master, SSP Enabled */
  
  /*
    Previous initialisation for Loopback mode:
    port->CR1 = SSPCR1_LBM | SSPCR1_SSE;

    Previous initialisation for Slave mode (Currently not implemented!)
   */
  
  if(initVars->Slave){
    /* Initialise in slave mode */
    
    if ( port->CR1 & SSPCR1_SSE ){
    /* The slave bit can't be set until SSE bit is zero. */
      port->CR1 &= ~SSPCR1_SSE;
    }
    
      port->CR1 = SSPCR1_MS;    /* Enable slave bit first */
      port->CR1 |= SSPCR1_SSE;  /* Enable SSP */
      
  }else{
    /* Initialise in master mode */
    port->CR1 = SSPCR1_SSE;
  }

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  port->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;    

    return;
}

/*****************************************************************************
** Function name:		SSP_Send
**
** Descriptions:		Send a block of data to the SSP port, the 
**						first parameter is the buffer pointer, the 2nd 
**						parameter is the block length.
**
** parameters:			port #, buffer pointer, and the block length
** Returned value:		None
** 
*****************************************************************************/
void SSP_Send( uint8_t portNum, uint8_t *buf, uint32_t Length )
{
  uint32_t i;
  uint8_t Dummy = Dummy;
    
  for ( i = 0; i < Length; i++ )
  {
	if ( portNum == 0 )
	{
	/* Move on only if NOT busy and TX FIFO not full. */
	  while ( (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
	  LPC_SSP0->DR = *buf;
	buf++;
#if !LOOPBACK_MODE
	  while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
	/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO 
	on MISO. Otherwise, when SSP0Receive() is called, previous data byte
	is left in the FIFO. */
	  Dummy = LPC_SSP0->DR;
#else
	/* Wait until the Busy bit is cleared. */
	  while ( LPC_SSP0->SR & SSPSR_BSY );
#endif
  }
	else
	{
	  /* Move on only if NOT busy and TX FIFO not full. */
	  while ( (LPC_SSP1->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
	  LPC_SSP1->DR = *buf;
	  buf++;
#if !LOOPBACK_MODE
	  while ( (LPC_SSP1->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
	  /* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO 
	  on MISO. Otherwise, when SSP0Receive() is called, previous data byte
	  is left in the FIFO. */
	  Dummy = LPC_SSP1->DR;
#else
	  /* Wait until the Busy bit is cleared. */
	  while ( LPC_SSP1->SR & SSPSR_BSY );
#endif
	}
  }
  return; 
}

/*****************************************************************************
** Function name:		SSP_Receive
** Descriptions:		the module will receive a block of data from 
**						the SSP, the 2nd parameter is the block 
**						length.
** parameters:			port #, buffer pointer, and block length
** Returned value:		None
** 
*****************************************************************************/
void SSP_Receive( uint8_t portNum, uint8_t *buf, uint32_t Length )
{
  uint32_t i;
 
  for ( i = 0; i < Length; i++ )
  {
	/* As long as Receive FIFO is not empty, I can always receive. */
	/* If it's a loopback test, clock is shared for both TX and RX,
	no need to write dummy byte to get clock to get the data */
	/* if it's a peer-to-peer communication, SSPDR needs to be written
	before a read can take place. */
	if ( portNum == 0 )
	{
#if !LOOPBACK_MODE
#if SSP_SLAVE
	  while ( !(LPC_SSP0->SR & SSPSR_RNE) );
#else
	  LPC_SSP0->DR = 0xFF;
	/* Wait until the Busy bit is cleared */
	  while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
#endif
#else
	  while ( !(LPC_SSP0->SR & SSPSR_RNE) );
#endif
	  *buf = LPC_SSP0->DR;
	  buf++;
	}
	else
	{
#if !LOOPBACK_MODE
#if SSP_SLAVE || SSP1_SLAVE
	  //LPC_SSP1->DR = 0x00;
	  while ( !(LPC_SSP1->SR & SSPSR_RNE) );
#else
	  LPC_SSP1->DR = 0x00;
	  /* Wait until the Busy bit is cleared */
	  while ( (LPC_SSP1->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
#endif
#else
	  while ( !(LPC_SSP1->SR & SSPSR_RNE) );
#endif
	  *buf = LPC_SSP1->DR;
	buf++;
	}
  }
  return; 
}



/*****************************************************************************
** Function name:		SSP_RX_Flush
** Descriptions:		The module will empty RX FIFO
** parameters:			port
** Returned value:		None
** 
*****************************************************************************/
void SSP_RX_Flush( uint8_t portNum )
{
	uint32_t buf;

	if (portNum == 0){
		while ( (LPC_SSP0->SR & SSPSR_RNE) ){
			buf = LPC_SSP0->DR;
		}
	}
	else {
		while ( (LPC_SSP1->SR & SSPSR_RNE) ){
			buf = LPC_SSP1->DR;
		}
	}
}

/******************************************************************************
**                            End Of File
******************************************************************************/

#endif