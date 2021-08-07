#ifndef __ARCH_H__
#define __ARCH_H__

struct Context {
  // TODO: fix the order of these members to match trap.S
  uintptr_t hi, gpr[32], epc, cause, lo, status;
  void *pdir;
};

#define GPR1 gpr[2] // v0
#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
