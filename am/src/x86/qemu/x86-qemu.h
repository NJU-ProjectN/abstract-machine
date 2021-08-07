#include <x86/x86.h>

#define PML4_ADDR  0x1000
#define PDPT_ADDR  0x2000

#define NR_SEG         6       // GDT size
#define SEG_KCODE      1       // Kernel code
#define SEG_KDATA      2       // Kernel data/stack
#define SEG_UCODE      3       // User code
#define SEG_UDATA      4       // User data/stack
#define SEG_TSS        5       // Global unique task state segement

#define NR_IRQ         256     // IDT size

#ifndef __ASSEMBLER__

#include <am.h>
#include <klib-macros.h>

struct kernel_stack {
  uint8_t stack[8192];
};

static inline void *stack_top(struct kernel_stack *stk) {
  return stk->stack + sizeof(stk->stack);
}

struct mmu_config {
  int ptlevels, pgsize;
  struct ptinfo {
    const char *name;
    uintptr_t mask;
    int shift, bits;
  } pgtables[];
};

struct vm_area {
  Area area;
  int kernel;
};

void __am_iret(Context *ctx);

struct cpu_local {
  AddrSpace *uvm;
#if __x86_64__
  SegDesc gdt[NR_SEG + 1];
  TSS64 tss;
#else
  SegDesc gdt[NR_SEG];
  TSS32 tss;
#endif
  struct kernel_stack stack;
};

#if __x86_64__
struct trap_frame {
  Context saved_context;
  uint64_t irq, errcode;
  uint64_t rip, cs, rflags, rsp, ss;
};
#else
struct trap_frame {
  Context saved_context;
  uint32_t irq, errcode;
  uint32_t eip, cs, eflags, esp, ss;
};
#endif

extern volatile uint32_t *__am_lapic;
extern int __am_ncpu;
extern struct cpu_local __am_cpuinfo[MAX_CPU];

#define CPU (&__am_cpuinfo[cpu_current()])

#define bug_on(cond) \
  do { \
    if (cond) panic("internal error (likely a bug in AM)"); \
  } while (0)

#define bug() bug_on(1)

// apic utils
void __am_lapic_eoi();
void __am_ioapic_init();
void __am_lapic_bootap(uint32_t cpu, void *address);
void __am_ioapic_enable(int irq, int cpu);

// x86-specific operations
void __am_bootcpu_init();
void __am_percpu_init();
Area __am_heap_init();
void __am_lapic_init();
void __am_othercpu_entry();
void __am_percpu_initirq();
void __am_percpu_initgdt();
void __am_percpu_initlapic();
void __am_stop_the_world();

#endif
