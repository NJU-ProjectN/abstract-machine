#ifndef ARCH_H__
#define ARCH_H__

struct Context {
  void    *cr3;
  uint64_t rax, rbx, rcx, rdx,
           rbp, rsi, rdi,
           r8, r9, r10, r11,
           r12, r13, r14, r15,
           rip, cs, rflags,
           rsp, ss, rsp0;
};


#define GPR1 rdi
#define GPR2 rsi
#define GPR3 rdx
#define GPR4 rcx
#define GPRx rax

#endif
