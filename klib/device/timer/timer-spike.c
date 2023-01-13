#include <am.h>

static uint64_t boot_time = 0;

#define CLINT_MMIO 0x2000000ul
#define TIME_BASE 0xbff8

static uint64_t read_time() {
  uint32_t lo = *(volatile uint32_t *)(CLINT_MMIO + TIME_BASE + 0);
  uint32_t hi = *(volatile uint32_t *)(CLINT_MMIO + TIME_BASE + 4);
  uint64_t time = ((uint64_t)hi << 32) | lo;
  return time / 10;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime->us = read_time() - boot_time;
}

void __am_timer_init() {
  boot_time = read_time();
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
