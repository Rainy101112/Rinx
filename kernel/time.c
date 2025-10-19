/*
 *
 *      time.c
 *      Time stamp
 *
 *      2024/6/29 By MicroFish
 *      Based on GPL-3.0 open source agreement
 *      Rinx Kernel project.
 *
 */

#include "cmos.h"
#include "common.h"

uint32_t is_leap_year(uint32_t year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint32_t get_days_in_month(uint32_t year, uint32_t month)
{
    static const uint8_t days_in_month[] = {
        31, 28, 31, 30, 31, 30, 
        31, 31, 30, 31, 30, 31
    };
    
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    
    if (month >= 1 && month <= 12) {
        return days_in_month[month - 1];
    }
    
    return 0;
}

uint64_t get_days_since_epoch(uint32_t year)
{
    uint64_t total_days = 0;
    uint32_t y;
    
    for (y = 1970; y < year; y++) {
        total_days += is_leap_year(y) ? 366 : 365;
    }
    
    return total_days;
}

uint32_t get_days_since_year_start(uint32_t year, uint32_t month)
{
    uint32_t total_days = 0;
    uint32_t m;
    
    /* 累加前几个月的天数 */
    for (m = 1; m < month; m++) {
        total_days += get_days_in_month(year, m);
    }
    
    return total_days;
}

uint64_t calculate_unix_timestamp(void)
{
    uint32_t year, month, day, hour, minute, second;
    uint64_t total_seconds = 0;
    
    year = get_year();
    month = get_mon_hex();
    day = get_day_of_month();
    hour = get_hour_hex();
    minute = get_min_hex();
    second = get_sec_hex();

    total_seconds = get_days_since_epoch(year) * 86400ULL;
    total_seconds += get_days_since_year_start(year, month) * 86400ULL;
    total_seconds += (day - 1) * 86400ULL;
    total_seconds += hour * 3600ULL;
    total_seconds += minute * 60ULL;
    total_seconds += second;

    return total_seconds;
}

uint64_t get_current_time(void)
{
    return calculate_unix_timestamp();
}
