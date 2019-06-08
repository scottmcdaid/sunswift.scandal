/* --------------------------------------------------------------------------
	Scandal EEPROM Code for MSP
	File name: scandal_eeprom.c
	Author: David Snowdon

    Copyright (C) David Snowdon, 2003. 
 
    Implementation of Scandal EEPROM functions for MSP430. 
 
	Date: 4/7/03
   -------------------------------------------------------------------------- */ 
   
/* 
 * This file is part of Scandal.
 * 
 * Scandal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * Scandal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with Scandal.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <msp430.h>
#include <signal.h>
#include "scandal_eeprom.h"
#include "scandal_led.h"

/* Notes on flash programming: 
   David Snowdon, 5/4/2008
   There seems to be a problem in here with the system resetting just after
   a flash operation. It may make it half way through programming, or it 
   may not make it through at all. 
   In either case, we have problems with the flash not being completely
   written. 
   To rectify this, we should a) do checksums and b) go through this code
   again with a fine-toothed comb. 
*/ 

/* Segment A is assigned to Scandal configuration, segment B is assigned to user configuration */

void 	sc_init_eeprom(void){
  FCTL3 = FWKEY + LOCK; 
  FCTL2 = FWKEY + FSSEL0 + (40 - 1); 
  /* MCLK/20 for Flash Timing Generator, assuming a 7.3728 MHz crystal
     gives us a 368kHz clock */ 
  /* Should be between 257 and 476 kHz according to section 5.3.1 
     of the user manual */ 
  
}

void sc_read_conf(scandal_config*	conf){
	u16	i;
	u08*	ptr;
	u08*    flash_ptr;
	
	ptr = (u08*)conf;
	flash_ptr = (u08*)0x1080;                 /* Segment A */

	/* Read the flash segment */
	for(i=0;(i<(sizeof(scandal_config))) &&
		    (i < 128);i++)
	  *ptr++ = *flash_ptr++;

	/* Read segment B */
	if(i < sizeof(scandal_config)){
	  flash_ptr = (u08*)0x1000;
		for(;(i<(sizeof(scandal_config))); i++){
			*ptr++ = *flash_ptr++;
		}
	}
}

void sc_write_conf(scandal_config*	conf){
	u16	i;
	uint8_t*    ptr;
	uint8_t*    flash_ptr;
	uint16_t saved_sr;


	saved_sr = READ_SR;
	dint();

	ptr = (u08*)conf;
	flash_ptr = (u08*)0x1080;                 /* Segment A */
	
	/* Erase the flash segment */
	FCTL1 = FWKEY + ERASE;
	FCTL3 = FWKEY;
	*flash_ptr = 0;                           /* Dummy write erases the 
						     segment */

	/* Write the flash segment */
	for(i=0;i<(sizeof(scandal_config)) && (i < 128); i++){
	  FCTL3 = FWKEY; 
	  FCTL1 = FWKEY + WRT; /* Set for write operation */
	  *flash_ptr++ = *ptr++;
	  FCTL1 = FWKEY; 
	  FCTL3 = FWKEY + LOCK; 
	}

	if(i < sizeof(scandal_config)){
		flash_ptr = (u08*)0x1000;                 /* Segment B */
		
		/* Erase the flash segment */
		FCTL1 = FWKEY + ERASE;
		FCTL3 = FWKEY;
		*flash_ptr = 0;                           
		
		/* Write the flash segment */
		FCTL1 = FWKEY + WRT;                      
		for(;i<(sizeof(scandal_config));i++){
		  FCTL3 = FWKEY; 
		  FCTL1 = FWKEY + WRT; /* Set for write operation */
		  *flash_ptr++ = *ptr++;
		  FCTL1 = FWKEY; 
		  FCTL3 = FWKEY + LOCK; 
		}
	}

	if((saved_sr & GIE) != 0) {
	  eint();
	}

}

void sc_user_eeprom_read_block(u32 loc, u08* data, u08 length){
  u16	i;
  u08*    flash_ptr;
  
  flash_ptr = (u08*)0x1000;
  for(i=0;i<length; i++){
    *data++ = *flash_ptr++;
  }
}

//#warning "sc_user_eeprom_write_block implementation presently ignores the loc parameter"
void sc_user_eeprom_write_block(u32 loc, u08* data, u08 length){
	u16	i;
	u08*    flash_ptr;
	uint16_t saved_sr;

	saved_sr = READ_SR;
	dint(); 
	
	{
	  volatile int blerg; 

	  for(blerg = 0; blerg < 10000; blerg++)
	    ;
	} 

	  


#if SIZEOF_SCANDAL_CONFIG <= 128
	flash_ptr = (u08*)0x1000;                 /* Segment B */
  
	/* Erase the flash segment */
	FCTL1 = FWKEY + ERASE;
	FCTL3 = FWKEY;
	*flash_ptr = 0;                           
	
 	/* Write the flash segment */
	FCTL1 = FWKEY + WRT;                      
	for(i=0;i<length;i++){
	  FCTL3 = FWKEY; 
	  FCTL1 = FWKEY + WRT; /* Set for write operation */
	  *flash_ptr++ = *data++;
	  FCTL1 = FWKEY; 
	  FCTL3 = FWKEY + LOCK; 
	}
#else
#warning USER CONFIG WILL NOT BE WRITTEN TO EEPROM
#endif
	if(saved_sr & GIE)
	  eint(); 
}
