#include <stdatomic.h>
#include "platform.h"

int __am_mpe_init = 0;
extern bool __am_has_ioe;
void __am_ioe_init();

bool mpe_init(void (*entry)()) {
  __am_mpe_init = 1;

  int sync_pipe[2];
  assert(0 == pipe(sync_pipe));

  for (int i = 1; i < cpu_count(); i++) {
    if (fork() == 0) {
      char ch;
      assert(read(sync_pipe[0], &ch, 1) == 1);
      assert(ch == '+');
      close(sync_pipe[0]); close(sync_pipe[1]);

      thiscpu->cpuid = i;
      __am_init_timer_irq();
      entry();
    }
  }

  if (__am_has_ioe) {
    __am_ioe_init();
  }

  for (int i = 1; i < cpu_count(); i++) {
    assert(write(sync_pipe[1], "+", 1) == 1);
  }
  close(sync_pipe[0]); close(sync_pipe[1]);
  
  entry();
  panic("MP entry should not return\n");
}

int cpu_count() {
  extern int __am_ncpu;
  return __am_ncpu;
}

int cpu_current() {
  return thiscpu->cpuid;
}

int atomic_xchg(int *addr, int newval) {
  return atomic_exchange((int *)addr, newval);
}
