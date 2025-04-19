#include <am.h>
#include <nemu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);

void putch(char ch) {
  outb(0x10000000, ch);
}

__attribute__((noinline))
void halt(int code) {
  while (1);
}

void _trm_init() {
  int ret = main("");
  halt(ret);
}
