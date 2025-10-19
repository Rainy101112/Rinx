#ifndef INCLUDE_TIME_H_
#define INCLUDE_TIME_H_

#include "stdint.h"
#include "stddef.h"

typedef unsigned long time_t;
typedef unsigned long time64_t;

uint64_t get_current_time(void);

#endif // INCLUDE_TIME_H_
