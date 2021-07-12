#include <am.h>
#include <mycpu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {

}

void halt(int code) {
  while (1);
}

void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}
