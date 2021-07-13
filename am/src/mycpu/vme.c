#include <am.h>

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  return false;
}

void protect(AddrSpace *as) {
}

void unprotect(AddrSpace *as) {
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  return NULL;
}
