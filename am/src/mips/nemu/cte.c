#include <am.h>
#include <mips/mips32.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

static inline bool get_CU0(Context *c) { return (c->status >> 28) & 0x1; }

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    uint32_t ex_code = 0;
    switch (ex_code) {
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

#define EX_ENTRY 0x80000180

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  const uint32_t j_opcode = 0x08000000;
  uint32_t instr = j_opcode | (((uint32_t)__am_asm_trap >> 2) & 0x3ffffff);
  *(uint32_t *)EX_ENTRY = instr;
  *(uint32_t *)(EX_ENTRY + 4) = 0;  // delay slot
  *(uint32_t *)0x80000000 = instr;  // TLB refill exception
  *(uint32_t *)(0x80000000 + 4) = 0;  // delay slot

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
  asm volatile("syscall 1");
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
