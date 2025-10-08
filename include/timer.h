/*
 *
 *      timer.h
 *      Timer header file
 *
 *      2025/2/17 By MicroFish
 *      Based on GPL-3.0 open source agreement
 *      Rinx Kernel project.
 *
 */

#ifndef INCLUDE_TIMER_H_
#define INCLUDE_TIMER_H_

#include "stdint.h"

/* Millisecond-based delay functions */
void msleep(uint64_t ms);

/* Nanosecond-based delay function */
void nsleep(uint64_t ns);

#endif // INCLUDE_TIMER_H_
