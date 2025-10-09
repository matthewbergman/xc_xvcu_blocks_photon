/*
 * counter.c
 *
 *  Created on: Jul 23, 2021
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#include <stdint.h>

void counter_init(uint16_t* counter)
{
	*counter = 0;
}

void counter_inc(uint16_t* counter)
{
	if (*counter < 0xFFFF)
		(*counter)++;
}

void counter_dec(uint16_t* counter)
{
	if (*counter > 0)
		(*counter)--;
}
