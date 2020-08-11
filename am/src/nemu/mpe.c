#include <am.h>

bool mpe_init(void (*entry)()) {
  return false;
}

int cpu_count() {
  return 1;
}

int cpu_current() {
  return 0;
}

int atomic_xchg(int *addr, intptr_t newval) {
  return 0;
}
