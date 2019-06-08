/***********************************************************************//**
 * @file		lpc17xx_gpmsp430.h
 * @brief		Contains all macro definitions and function prototypes
 * 				support for GPIO firmware library on LPC17xx
 * @version		3.0
 * @date		18. June. 2010
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
 **************************************************************************/

#ifndef LPC17XX_GPIO_H_
#define LPC17XX_GPIO_H_

/* Includes ------------------------------------------------------------------- */
#include <cmsis/LPC17xx.h>
#include <arch/types.h>

/* Public Macros -------------------------------------------------------------- */

/** Fast GPIO port 0 byte accessible definition */
#define GPIO0_Byte	((GPIO_Byte_TypeDef *)(LPC_GPIO0_BASE))
/** Fast GPIO port 1 byte accessible definition */
#define GPIO1_Byte	((GPIO_Byte_TypeDef *)(LPC_GPIO1_BASE))
/** Fast GPIO port 2 byte accessible definition */
#define GPIO2_Byte	((GPIO_Byte_TypeDef *)(LPC_GPIO2_BASE))
/** Fast GPIO port 3 byte accessible definition */
#define GPIO3_Byte	((GPIO_Byte_TypeDef *)(LPC_GPIO3_BASE))
/** Fast GPIO port 4 byte accessible definition */
#define GPIO4_Byte	((GPIO_Byte_TypeDef *)(LPC_GPIO4_BASE))


/** Fast GPIO port 0 half-word accessible definition */
#define GPIO0_HalfWord	((GPIO_HalfWord_TypeDef *)(LPC_GPIO0_BASE))
/** Fast GPIO port 1 half-word accessible definition */
#define GPIO1_HalfWord	((GPIO_HalfWord_TypeDef *)(LPC_GPIO1_BASE))
/** Fast GPIO port 2 half-word accessible definition */
#define GPIO2_HalfWord	((GPIO_HalfWord_TypeDef *)(LPC_GPIO2_BASE))
/** Fast GPIO port 3 half-word accessible definition */
#define GPIO3_HalfWord	((GPIO_HalfWord_TypeDef *)(LPC_GPIO3_BASE))
/** Fast GPIO port 4 half-word accessible definition */
#define GPIO4_HalfWord	((GPIO_HalfWord_TypeDef *)(LPC_GPIO4_BASE))

//converts bit number into 32-bit patten
#define BIT(x) (1<<x)

/* Public Types --------------------------------------------------------------- */
/** @defgroup GPIO_Public_Types GPIO Public Types
 * @{
 */

/**
 * @brief Fast GPIO port byte type definition
 */
typedef struct {
	__IO uint8_t FIODIR[4];		/**< FIO direction register in byte-align */
	   uint32_t RESERVED0[3];	/**< Reserved */
	__IO uint8_t FIOMASK[4];	/**< FIO mask register in byte-align */
	__IO uint8_t FIOPIN[4];		/**< FIO pin register in byte align */
	__IO uint8_t FIOSET[4];		/**< FIO set register in byte-align */
	__O  uint8_t FIOCLR[4];		/**< FIO clear register in byte-align */
} GPIO_Byte_TypeDef;


/**
 * @brief Fast GPIO port half-word type definition
 */
typedef struct {
	__IO uint16_t FIODIRL;		/**< FIO direction register lower halfword part */
	__IO uint16_t FIODIRU;		/**< FIO direction register upper halfword part */
	   uint32_t RESERVED0[3];	/**< Reserved */
	__IO uint16_t FIOMASKL;		/**< FIO mask register lower halfword part */
	__IO uint16_t FIOMASKU;		/**< FIO mask register upper halfword part */
	__IO uint16_t FIOPINL;		/**< FIO pin register lower halfword part */
	__IO uint16_t FIOPINU;		/**< FIO pin register upper halfword part */
	__IO uint16_t FIOSETL;		/**< FIO set register lower halfword part */
	__IO uint16_t FIOSETU;		/**< FIO set register upper halfword part */
	__O  uint16_t FIOCLRL;		/**< FIO clear register lower halfword part */
	__O  uint16_t FIOCLRU;		/**< FIO clear register upper halfword part */
} GPIO_HalfWord_TypeDef;

/* Public Functions ----------------------------------------------------------- */

/* Scandal GPIO style ------------------------------- */
void GPIO_Init(void);
void GPIO_SetDir(uint8_t portNum, uint8_t bitPosi, uint8_t dir);
void GPIO_SetValue(uint8_t portNum, uint8_t bitPosi, uint8_t bitValue);
uint8_t GPIO_ReadValue(uint8_t portNum, uint8_t bitPosi);
void GPIO_ToggleValue( uint8_t portNum, uint8_t bitPosi);


/* GPIO Interupts, still using CMSIS style */
void GPIO_IntCmd(uint8_t portNum, uint32_t bitValue, uint8_t edgeState);
FunctionalState GPIO_GetIntStatus(uint8_t portNum, uint32_t pinNum, uint8_t edgeState);
void GPIO_ClearInt(uint8_t portNum, uint32_t bitValue);

#endif /* LPC17XX_GPIO_H_ */

