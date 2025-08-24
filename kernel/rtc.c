#include "rtc.h"
#include "io.h"   // inb/outb
// Lecture CMOS (ports 0x70/0x71)
static uint8_t rtc_read(uint8_t reg){
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t b){ return ((b>>4)*10) + (b & 0x0F); }

void rtc_init(void){ /* Rien de lourd nécessaire pour l'instant */ }

void rtc_get_time(rtc_time_t* t){
    // simple read BCD values, convert to bin
    uint8_t s = rtc_read(0x00);
    uint8_t m = rtc_read(0x02);
    uint8_t h = rtc_read(0x04);
    uint8_t day = rtc_read(0x07);
    uint8_t mon = rtc_read(0x08);
    uint8_t yr  = rtc_read(0x09);
    t->sec = bcd_to_bin(s);
    t->min = bcd_to_bin(m);
    t->hour = bcd_to_bin(h);
    t->day = bcd_to_bin(day);
    t->month = bcd_to_bin(mon);
    t->year = bcd_to_bin(yr);
}
