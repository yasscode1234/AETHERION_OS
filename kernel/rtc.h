#ifndef RTC_H
#define RTC_H
#include "types.h"
typedef struct { uint8_t sec,min,hour,day,month,year; } rtc_time_t;
void rtc_init(void);
void rtc_get_time(rtc_time_t* t);
#endif
