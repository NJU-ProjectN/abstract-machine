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
#ifdef __x86_64__
  // skip the red zone of the stack frame, see the amd64 ABI manual for details
  uint8_t redzone[128];
#endif
};

#ifdef __x86_64__
#define GPR1 uc.uc_mcontext.gregs[REG_RDI]
#define GPR2 uc.uc_mcontext.gregs[REG_RSI]
#define GPR3 uc.uc_mcontext.gregs[REG_RDX]
#define GPR4 uc.uc_mcontext.gregs[REG_RCX]
#define GPRx uc.uc_mcontext.gregs[REG_RAX]
#elif defined(__aarch64__)
#define GPR1 uc.uc_mcontext.regs[0]
#define GPR2 uc.uc_mcontext.regs[1]
#define GPR3 uc.uc_mcontext.regs[2]
#define GPR4 uc.uc_mcontext.regs[3]
#define GPRx uc.uc_mcontext.regs[0]
#elif defined(__riscv)
// FIXME: may be wrong
#define GPR1 uc.uc_mcontext.__gregs[REG_A0]
#define GPR2 uc.uc_mcontext.__gregs[REG_A0+1]
#define GPR3 uc.uc_mcontext.__gregs[REG_A0+2]
#define GPR4 uc.uc_mcontext.__gregs[REG_A0+3]
#define GPRx uc.uc_mcontext.__gregs[REG_A0+4]
#else
#error Unsupported architecture
#endif

#undef __USE_GNU

#endif
