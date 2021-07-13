#include "x86-qemu.h"

static Context* (*user_handler)(Event, Context*) = NULL;
#if __x86_64__
static GateDesc64 idt[NR_IRQ];
#define GATE GATE64
#else
static GateDesc32 idt[NR_IRQ];
#define GATE GATE32
#endif

#define IRQHANDLE_DECL(id, dpl, err) \
  void __am_irq##id();

IRQS(IRQHANDLE_DECL)
void __am_irqall();
void __am_kcontext_start();

void __am_irq_handle(struct trap_frame *tf) {
  Context *saved_ctx = &tf->saved_context;
  Event ev = {
    .event = EVENT_NULL,
    .cause = 0, .ref = 0,
    .msg = "(no message)",
  };

#if __x86_64
  saved_ctx->rip    = tf->rip;
  saved_ctx->cs     = tf->cs;
  saved_ctx->rflags = tf->rflags;
  saved_ctx->rsp    = tf->rsp;
  saved_ctx->rsp0   = CPU->tss.rsp0;
  saved_ctx->ss     = tf->ss;
#else
  saved_ctx->eip    = tf->eip;
  saved_ctx->cs     = tf->cs;
  saved_ctx->eflags = tf->eflags;
  saved_ctx->esp0   = CPU->tss.esp0;
  saved_ctx->ss3    = USEL(SEG_UDATA);
  // no ss/esp saved for DPL_KERNEL
  saved_ctx->esp = (tf->cs & DPL_USER ? tf->esp : (uint32_t)(tf + 1) - 8);
#endif
  saved_ctx->cr3    = (void *)get_cr3();

  #define IRQ    T_IRQ0 +
  #define MSG(m) ev.msg = m;

  if (IRQ 0 <= tf->irq && tf->irq < IRQ 32) {
    __am_lapic_eoi();
  }

  switch (tf->irq) {
    case IRQ 0: MSG("timer interrupt (lapic)")
      ev.event = EVENT_IRQ_TIMER; break;
    case IRQ 1: MSG("I/O device IRQ1 (keyboard)")
      ev.event = EVENT_IRQ_IODEV; break;
    case IRQ 4: MSG("I/O device IRQ4 (COM1)")
      ev.event = EVENT_IRQ_IODEV; break;
    case EX_SYSCALL: MSG("int $0x80 system call")
      ev.event = EVENT_SYSCALL; break;
    case EX_YIELD: MSG("int $0x81 yield")
      ev.event = EVENT_YIELD; break;
    case EX_DE: MSG("DE #0 divide by zero")
      ev.event = EVENT_ERROR; break;
    case EX_UD: MSG("UD #6 invalid opcode")
      ev.event = EVENT_ERROR; break;
    case EX_NM: MSG("NM #7 coprocessor error")
      ev.event = EVENT_ERROR; break;
    case EX_DF: MSG("DF #8 double fault")
      ev.event = EVENT_ERROR; break;
    case EX_TS: MSG("TS #10 invalid TSS")
      ev.event = EVENT_ERROR; break;
    case EX_NP: MSG("NP #11 segment/gate not present")
      ev.event = EVENT_ERROR; break;
    case EX_SS: MSG("SS #12 stack fault")
      ev.event = EVENT_ERROR; break;
    case EX_GP: MSG("GP #13, general protection fault")
      ev.event = EVENT_ERROR; break;
    case EX_PF: MSG("PF #14, page fault, @cause: PROT_XXX")
      ev.event = EVENT_PAGEFAULT;
      if (tf->errcode & 0x1) ev.cause |= MMAP_NONE;
      if (tf->errcode & 0x2) ev.cause |= MMAP_WRITE;
      else                   ev.cause |= MMAP_READ;
      ev.ref = get_cr2();
      break;
    default: MSG("unrecognized interrupt/exception")
      ev.event = EVENT_ERROR;
      ev.cause = tf->errcode;
      break;
  }

  Context *ret_ctx = user_handler(ev, saved_ctx);
  panic_on(!ret_ctx, "returning to NULL context");

  if (ret_ctx->cr3) {
    set_cr3(ret_ctx->cr3);
#if __x86_64__
    CPU->tss.rsp0 = ret_ctx->rsp0;
#else
    CPU->tss.ss0  = KSEL(SEG_KDATA);
    CPU->tss.esp0 = ret_ctx->esp0;
#endif
  }

  __am_iret(ret_ctx);
}

bool cte_init(Context *(*handler)(Event, Context *)) {
  panic_on(cpu_current() != 0, "init CTE in non-bootstrap CPU");
  panic_on(!handler, "no interrupt handler");

  for (int i = 0; i < NR_IRQ; i ++) {
    idt[i]  = GATE(STS_TG, KSEL(SEG_KCODE), __am_irqall,  DPL_KERN);
  }
#define IDT_ENTRY(id, dpl, err) \
    idt[id] = GATE(STS_TG, KSEL(SEG_KCODE), __am_irq##id, DPL_##dpl);
  IRQS(IDT_ENTRY)

  user_handler = handler;
  return true;
}

void yield() {
  interrupt(0x81);
}

bool ienabled() {
  return (get_efl() & FL_IF) != 0;
}

void iset(bool enable) {
  if (enable) sti();
  else cli();
}

void __am_panic_on_return() { panic("kernel context returns"); }

Context* kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *ctx = kstack.end - sizeof(Context);
  *ctx = (Context) { 0 };

#if __x86_64__
  ctx->cs     = KSEL(SEG_KCODE);
  ctx->rip    = (uintptr_t)__am_kcontext_start;
  ctx->rflags = FL_IF;
  ctx->rsp    = (uintptr_t)kstack.end;
#else
  ctx->ds     = KSEL(SEG_KDATA);
  ctx->cs     = KSEL(SEG_KCODE);
  ctx->eip    = (uintptr_t)__am_kcontext_start;
  ctx->eflags = FL_IF;
  ctx->esp    = (uintptr_t)kstack.end;
#endif

  ctx->GPR1 = (uintptr_t)arg;
  ctx->GPR2 = (uintptr_t)entry;

  return ctx;
}

void __am_percpu_initirq() {
  __am_ioapic_enable(IRQ_KBD, 0);
  __am_ioapic_enable(IRQ_COM1, 0);
  set_idt(idt, sizeof(idt));
}
