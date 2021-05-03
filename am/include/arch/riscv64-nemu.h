#ifndef ARCH_H__
#define ARCH_H__

struct Context {
  uintptr_t epc, cause, gpr[32], status;
  void *pdir;
};

#define GPR1 gpr[17] // a7
#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
