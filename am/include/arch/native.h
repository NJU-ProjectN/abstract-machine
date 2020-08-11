#ifndef ARCH_H__
#define ARCH_H__

#ifndef __USE_GNU
# define __USE_GNU
#endif

#include <ucontext.h>

struct Context {
  uintptr_t ksp;
  void *vm_head;
  ucontext_t uc;
  // skip the red zone of the stack frame, see the amd64 ABI manual for details
  uint8_t redzone[128];
};

#define GPR1 uc.uc_mcontext.gregs[REG_RDI]
#define GPR2 uc.uc_mcontext.gregs[REG_RSI]
#define GPR3 uc.uc_mcontext.gregs[REG_RDX]
#define GPR4 uc.uc_mcontext.gregs[REG_RCX]
#define GPRx uc.uc_mcontext.gregs[REG_RAX]

#undef __USE_GNU

#endif
