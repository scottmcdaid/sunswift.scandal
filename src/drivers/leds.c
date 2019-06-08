/* --------------------------------------------------------------------------
	Scandal LEDs
	File name: leds.c
	Author: Etienne Le Sueur
 
    Copyright (C) 2011 Etienne Le Sueur.

	Date: 29/08/2011
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

#include <scandal/types.h>
#include <scandal/leds.h>

#if defined(lpc11c14) || defined(lpc1768)
#include <project/driver_config.h>
#include <project/target_config.h>
#else
#if defined(msp430f149)
#include <project/hardware.h>
#include <arch/gpio.h>
#endif
#endif

//#include <arch/gpmsp430.h>

void red_led(u08 on) {
	if (on)
		GPIO_SetValue(RED_LED_PORT, RED_LED_BIT, 0);
	else
		GPIO_SetValue(RED_LED_PORT, RED_LED_BIT, 1);
}

void toggle_red_led(void) {
	GPIO_ToggleValue(RED_LED_PORT, RED_LED_BIT);
}

void yellow_led(u08 on) {
	if (on)
		GPIO_SetValue(YELLOW_LED_PORT, YELLOW_LED_BIT, 0);
	else
		GPIO_SetValue(YELLOW_LED_PORT, YELLOW_LED_BIT, 1);
}

void toggle_yellow_led(void) {
	GPIO_ToggleValue(YELLOW_LED_PORT, YELLOW_LED_BIT);
}
