#include "x86-qemu.h"

.globl __am_kcontext_start
__am_kcontext_start:
  // rdi = arg, rsi = entry
  pushq $__am_panic_on_return
  jmpq *%rsi

trap:
  cli
  subq  $48, %rsp
  pushq %r15
  pushq %r14
  pushq %r13
  pushq %r12
  pushq %r11
  pushq %r10
  pushq %r9
  pushq %r8
  pushq %rdi
  pushq %rsi
  pushq %rbp
  pushq %rdx
  pushq %rcx
  pushq %rbx
  pushq %rax
  pushq $0  // cr3, saved in __am_irq_handle

  movq  %rsp, %rdi
  call  __am_irq_handle

.globl __am_iret
__am_iret:
  movq  %rdi, %rsp
  movq  160(%rsp), %rax
  movw  %ax, %ds
  movw  %ax, %es
  addq  $8, %rsp
  popq  %rax
  popq  %rbx
  popq  %rcx
  popq  %rdx
  popq  %rbp
  popq  %rsi
  popq  %rdi
  popq  %r8
  popq  %r9
  popq  %r10
  popq  %r11
  popq  %r12
  popq  %r13
  popq  %r14
  popq  %r15
  iretq

#define NOERR     push $0
#define ERR
#define IRQ_DEF(id, dpl, err) \
  .globl __am_irq##id; __am_irq##id: cli;      err; push $id; jmp trap;
IRQS(IRQ_DEF)
  .globl  __am_irqall;  __am_irqall: cli; push $0; push $-1; jmp trap;
