/***********************************************************************//**
 * @file		lpc17xx_gpio.c
 * @brief		Contains all functions support for GPIO firmware library on LPC17xx
 * @version		2.0
 * @date		21. May. 2010
 * @author		NXP MCU SW Application Team
 **************************************************************************
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
 **********************************************************************/

/* Includes ------------------------------------------------------------------- */
#include <arch/gpmsp430.h>

/* Private Functions ---------------------------------------------------------- */

static LPC_GPIO_TypeDef *GPIO_GetPointer(uint8_t portNum);
//static GPIO_HalfWord_TypeDef *FIO_HalfWordGetPointer(uint8_t portNum);
//static GPIO_Byte_TypeDef *FIO_ByteGetPointer(uint8_t portNum);

/*********************************************************************//**
 * @brief		Get pointer to GPIO peripheral due to GPIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 4.
 * @return		Pointer to GPIO peripheral
 **********************************************************************/
static LPC_GPIO_TypeDef *GPIO_GetPointer(uint8_t portNum)
{
	LPC_GPIO_TypeDef *pGPIO = NULL;

	switch (portNum) {
	case 0:
		pGPIO = LPC_GPIO0;
		break;
	case 1:
		pGPIO = LPC_GPIO1;
		break;
	case 2:
		pGPIO = LPC_GPIO2;
		break;
	case 3:
		pGPIO = LPC_GPIO3;
		break;
	case 4:
		pGPIO = LPC_GPIO4;
		break;
	default:
		break;
	}

	return pGPIO;
}


/* Not used in Scandal


***********************************************************************
 * @brief		Get pointer to FIO peripheral in halfword accessible style
 * 				due to FIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 4.
 * @return		Pointer to FIO peripheral
 **********************************************************************
static GPIO_HalfWord_TypeDef *FIO_HalfWordGetPointer(uint8_t portNum)
{
	GPIO_HalfWord_TypeDef *pFIO = NULL;

	switch (portNum) {
	case 0:
		pFIO = GPIO0_HalfWord;
		break;
	case 1:
		pFIO = GPIO1_HalfWord;
		break;
	case 2:
		pFIO = GPIO2_HalfWord;
		break;
	case 3:
		pFIO = GPIO3_HalfWord;
		break;
	case 4:
		pFIO = GPIO4_HalfWord;
		break;
	default:
		break;
	}

	return pFIO;
}

***********************************************************************
 * @brief		Get pointer to FIO peripheral in byte accessible style
 * 				due to FIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 4.
 * @return		Pointer to FIO peripheral
 **********************************************************************
static GPIO_Byte_TypeDef *FIO_ByteGetPointer(uint8_t portNum)
{
	GPIO_Byte_TypeDef *pFIO = NULL;

	switch (portNum) {
	case 0:
		pFIO = GPIO0_Byte;
		break;
	case 1:
		pFIO = GPIO1_Byte;
		break;
	case 2:
		pFIO = GPIO2_Byte;
		break;
	case 3:
		pFIO = GPIO3_Byte;
		break;
	case 4:
		pFIO = GPIO4_Byte;
		break;
	default:
		break;
	}

	return pFIO;
}

*/

/* End of Private Functions --------------------------------------------------- */


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup GPIO_Public_Functions
 * @{
 */


/* GPIO ------------------------------------------------------------------------------ */

void GPIO_SetDir(uint8_t portNum, uint8_t bitPosi, uint8_t dir)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		// Enable Output
		if (dir) {
			pGPIO->FIODIR |= BIT(bitPosi);
		}
		// Enable Input
		else {
			pGPIO->FIODIR &= ~BIT(bitPosi);
		}
	}
}

void GPIO_SetValue(uint8_t portNum, uint8_t bitPosi, uint8_t bitValue)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		if (bitValue) {
			pGPIO->FIOSET |= BIT(bitPosi);
		}
		else {
			pGPIO->FIOCLR |= BIT(bitPosi);
		}
	}
}


uint8_t GPIO_ReadValue(uint8_t portNum, uint8_t bitPosi)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		return ((pGPIO->FIOPIN >> bitPosi) & 0x01);
	}

	return (0);
}

/*********************************************************************//**
 * @brief		Enable GPIO interrupt (just used for P0.0-P0.30, P2.0-P2.13)
 * @param[in]	portNum		Port number to read value, should be: 0 or 2
 * @param[in]	bitValue	Value that contains all bits on GPIO to enable,
 * 							in range from 0 to 0xFFFFFFFF.
 * @param[in]	edgeState	state of edge, should be:
 * 							- 0: Rising edge
 * 							- 1: Falling edge
 * @return		None
 **********************************************************************/
void GPIO_IntCmd(uint8_t portNum, uint32_t bitValue, uint8_t edgeState)
{
	if((portNum == 0)&&(edgeState == 0))
		LPC_GPIOINT->IO0IntEnR = bitValue;
	else if ((portNum == 2)&&(edgeState == 0))
		LPC_GPIOINT->IO2IntEnR = bitValue;
	else if ((portNum == 0)&&(edgeState == 1))
		LPC_GPIOINT->IO0IntEnF = bitValue;
	else if ((portNum == 2)&&(edgeState == 1))
		LPC_GPIOINT->IO2IntEnF = bitValue;
	else
		//Error
		while(1);
}

/*********************************************************************//**
 * @brief		Get GPIO Interrupt Status (just used for P0.0-P0.30, P2.0-P2.13)
 * @param[in]	portNum		Port number to read value, should be: 0 or 2
 * @param[in]	pinNum		Pin number, should be: 0..30(with port 0) and 0..13
 * 							(with port 2)
 * @param[in]	edgeState	state of edge, should be:
 * 							- 0: Rising edge
 * 							- 1: Falling edge
 * @return		Bool	could be:
 * 						- ENABLE: Interrupt has been generated due to a rising
 * 								edge on P0.0
 * 						- DISABLE: A rising edge has not been detected on P0.0
 **********************************************************************/
FunctionalState GPIO_GetIntStatus(uint8_t portNum, uint32_t pinNum, uint8_t edgeState)
{
	if((portNum == 0) && (edgeState == 0))//Rising Edge
		return (((LPC_GPIOINT->IO0IntStatR)>>pinNum)& 0x1);
	else if ((portNum == 2) && (edgeState == 0))
		return (((LPC_GPIOINT->IO2IntStatR)>>pinNum)& 0x1);
	else if ((portNum == 0) && (edgeState == 1))//Falling Edge
		return (((LPC_GPIOINT->IO0IntStatF)>>pinNum)& 0x1);
	else if ((portNum == 2) && (edgeState == 1))
		return (((LPC_GPIOINT->IO2IntStatF)>>pinNum)& 0x1);
	else
		//Error
		while(1);
}
/*********************************************************************//**
 * @brief		Clear GPIO interrupt (just used for P0.0-P0.30, P2.0-P2.13)
 * @param[in]	portNum		Port number to read value, should be: 0 or 2
 * @param[in]	bitValue	Value that contains all bits on GPIO to enable,
 * 							in range from 0 to 0xFFFFFFFF.
 * @return		None
 **********************************************************************/
void GPIO_ClearInt(uint8_t portNum, uint32_t bitValue)
{
	if(portNum == 0)
		LPC_GPIOINT->IO0IntClr = bitValue;
	else if (portNum == 2)
		LPC_GPIOINT->IO2IntClr = bitValue;
	else
		//Invalid portNum
		while(1);
}

void GPIO_Init(void) {
 //nothing to initialise...
}



void GPIO_ToggleValue(uint8_t portNum, uint8_t bitPosi) {
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		 pGPIO->FIOPIN ^= BIT(bitPosi);
	}
}
