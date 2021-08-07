#ifndef ARCH_H__
#define ARCH_H__

struct Context {
  uintptr_t gpr[1];
};

#define GPR1 gpr[0]
#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
