#include "x86-qemu.h"

struct cpu_local __am_cpuinfo[MAX_CPU] = {};
static void (* volatile user_entry)();
static int ap_ready = 0;

static void call_user_entry() {
  user_entry();
  panic("MPE entry should not return");
}

bool mpe_init(void (*entry)()) {
  user_entry = entry;
  boot_record()->jmp_code = 0x000bfde9; // (16-bit) jmp (0x7c00)
  for (int cpu = 1; cpu < __am_ncpu; cpu++) {
    boot_record()->is_ap = 1;
    __am_lapic_bootap(cpu, (void *)boot_record());
    while (xchg(&ap_ready, 0) != 1) {
      pause();
    }
  }
  call_user_entry();
  return true;
}

static void othercpu_entry() {
  __am_percpu_init();
  xchg(&ap_ready, 1);
  call_user_entry();
}

void __am_othercpu_entry() {
  stack_switch_call(stack_top(&CPU->stack), othercpu_entry, 0);
}

int cpu_count() {
  return __am_ncpu;
}

int cpu_current(void) {
  return __am_lapic[8] >> 24;
}

int atomic_xchg(int *addr, int newval) {
  return xchg(addr, newval);
}

void __am_stop_the_world() {
  boot_record()->jmp_code = 0x0000feeb; // (16-bit) jmp .
  for (int cpu_ = 0; cpu_ < __am_ncpu; cpu_++) {
    if (cpu_ != cpu_current()) {
      __am_lapic_bootap(cpu_, (void *)boot_record());
    }
  }
}
