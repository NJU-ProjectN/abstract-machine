#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <am.h>
#include <unistd.h>
#include <signal.h>
#include <klib.h>
#include <klib-macros.h>

void __am_get_example_uc(Context *r);
void __am_get_intr_sigmask(sigset_t *s);
int __am_is_sigmask_sti(sigset_t *s);
void __am_init_timer_irq();
void __am_pmem_map(void *va, void *pa, int prot);
void __am_pmem_unmap(void *va);

// per-cpu structure
typedef struct {
  void *vm_head;
  uintptr_t ksp;
  int cpuid;
  Event ev; // similar to cause register in mips/riscv
  uint8_t sigstack[8192];
} __am_cpu_t;
extern __am_cpu_t *__am_cpu_struct;
#define thiscpu __am_cpu_struct

#endif
