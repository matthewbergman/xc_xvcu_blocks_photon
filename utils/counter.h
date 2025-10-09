/*
 * counter.h
 *
 *  Created on: Jul 23, 2021
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#ifndef COMPONENTS_COUNTER_H_
#define COMPONENTS_COUNTER_H_

#include <stdint.h>

// Initialize the counter to 0
void counter_init(uint16_t* counter);

// Increment the counter by 1, not exceeding 0xFFFF
void counter_inc(uint16_t* counter);

// Decrement the counter by 1, not dropping below 0
void counter_dec(uint16_t* counter);

#endif
