#include <am.h>
#include <stdio.h>

void __am_platform_dummy();
void __am_exit_platform(int code);

void trm_init() {
  __am_platform_dummy();
}

void putch(char ch) {
  putchar(ch);
}

void halt(int code) {
  printf("Exit (%d)\n", code);
  __am_exit_platform(code);
  printf("Should not reach here!\n");
  while (1);
}

Area heap = {};
