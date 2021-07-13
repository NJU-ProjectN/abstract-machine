#include <sys/time.h>
#include <string.h>
#include "platform.h"

#define TIMER_HZ 100
#define SYSCALL_INSTR_LEN 7

static Context* (*user_handler)(Event, Context*) = NULL;

void __am_kcontext_start();
void __am_switch(Context *c);
int __am_in_userspace(void *addr);
void __am_pmem_protect();
void __am_pmem_unprotect();

void __am_panic_on_return() { panic("should not reach here\n"); }

static void irq_handle(Context *c) {
  c->vm_head = thiscpu->vm_head;
  c->ksp = thiscpu->ksp;

  if (thiscpu->ev.event == EVENT_ERROR) {
    uintptr_t rip = c->uc.uc_mcontext.gregs[REG_RIP];
    printf("Unhandle signal '%s' at rip = %p, badaddr = %p, cause = 0x%x\n",
        thiscpu->ev.msg, rip, thiscpu->ev.ref, thiscpu->ev.cause);
    assert(0);
  }
  c = user_handler(thiscpu->ev, c);
  assert(c != NULL);

  __am_switch(c);

  // magic call to restore context
  void (*p)(Context *c) = (void *)(uintptr_t)0x100008;
  p(c);
  __am_panic_on_return();
}

static void setup_stack(uintptr_t event, ucontext_t *uc) {
  void *rip = (void *)uc->uc_mcontext.gregs[REG_RIP];
  extern uint8_t _start, _etext;
  int trap_from_user = __am_in_userspace(rip);
  int signal_safe = IN_RANGE(rip, RANGE(&_start, &_etext)) || trap_from_user ||
    // Hack here: "+13" points to the instruction after syscall. This is the
    // instruction which will trigger the pending signal if interrupt is enabled.
    (rip == (void *)&sigprocmask + 13);

  if (((event == EVENT_IRQ_IODEV) || (event == EVENT_IRQ_TIMER)) && !signal_safe) {
    // Shared libraries contain code which are not reenterable.
    // If the signal comes when executing code in shared libraries,
    // the signal handler can not call any function which is not signal-safe,
    // else the behavior is undefined (may be dead lock).
    // To handle this, we just refuse to handle the signal and return directly
    // to pretend missing the interrupt.
    // See man 7 signal-safety for more information.
    return;
  }

  if (trap_from_user) __am_pmem_unprotect();

  // skip the instructions causing SIGSEGV for syscall
  if (event == EVENT_SYSCALL) { rip += SYSCALL_INSTR_LEN; }
  uc->uc_mcontext.gregs[REG_RIP] = (uintptr_t)rip;

  // switch to kernel stack if we were previously in user space
  uintptr_t rsp = trap_from_user ? thiscpu->ksp : uc->uc_mcontext.gregs[REG_RSP];
  rsp -= sizeof(Context);
  // keep (rsp + 8) % 16 == 0 to support SSE
  if ((rsp + 8) % 16 != 0) rsp -= 8;
  Context *c = (void *)rsp;

  // save the context on the stack
  c->uc = *uc;

  // disable interrupt
  __am_get_intr_sigmask(&uc->uc_sigmask);

  // call irq_handle after returning from the signal handler
  uc->uc_mcontext.gregs[REG_RDI] = (uintptr_t)c;
  uc->uc_mcontext.gregs[REG_RIP] = (uintptr_t)irq_handle;
  uc->uc_mcontext.gregs[REG_RSP] = (uintptr_t)c;
}

static void iret(ucontext_t *uc) {
  Context *c = (void *)uc->uc_mcontext.gregs[REG_RDI];
  // restore the context
  *uc = c->uc;
  thiscpu->ksp = c->ksp;
  if (__am_in_userspace((void *)uc->uc_mcontext.gregs[REG_RIP])) __am_pmem_protect();
}

static void sig_handler(int sig, siginfo_t *info, void *ucontext) {
  thiscpu->ev = (Event) {0};
  thiscpu->ev.event = EVENT_ERROR;
  switch (sig) {
    case SIGUSR1: thiscpu->ev.event = EVENT_IRQ_IODEV; break;
    case SIGUSR2: thiscpu->ev.event = EVENT_YIELD; break;
    case SIGVTALRM: thiscpu->ev.event = EVENT_IRQ_TIMER; break;
    case SIGSEGV:
      if (info->si_code == SEGV_ACCERR) {
        switch ((uintptr_t)info->si_addr) {
          case 0x100000: thiscpu->ev.event = EVENT_SYSCALL; break;
          case 0x100008: iret(ucontext); return;
        }
      }
      if (__am_in_userspace(info->si_addr)) {
        assert(thiscpu->ev.event == EVENT_ERROR);
        thiscpu->ev.event = EVENT_PAGEFAULT;
        switch (info->si_code) {
          case SEGV_MAPERR: thiscpu->ev.cause = MMAP_READ; break;
          // we do not support mapped user pages with MMAP_NONE
          case SEGV_ACCERR: thiscpu->ev.cause = MMAP_WRITE; break;
          default: assert(0);
        }
        thiscpu->ev.ref = (uintptr_t)info->si_addr;
      }
      break;
  }

  if (thiscpu->ev.event == EVENT_ERROR) {
    thiscpu->ev.ref = (uintptr_t)info->si_addr;
    thiscpu->ev.cause = (uintptr_t)info->si_code;
    thiscpu->ev.msg = strsignal(sig);
  }
  setup_stack(thiscpu->ev.event, ucontext);
}

// signal handlers are inherited across fork()
static void install_signal_handler() {
  struct sigaction s;
  memset(&s, 0, sizeof(s));
  s.sa_sigaction = sig_handler;
  s.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
  __am_get_intr_sigmask(&s.sa_mask);

  int ret = sigaction(SIGVTALRM, &s, NULL);
  assert(ret == 0);
  ret = sigaction(SIGUSR1, &s, NULL);
  assert(ret == 0);
  ret = sigaction(SIGUSR2, &s, NULL);
  assert(ret == 0);
  ret = sigaction(SIGSEGV, &s, NULL);
  assert(ret == 0);
}

// setitimer() are inherited across fork(), should be called again from children
void __am_init_timer_irq() {
  iset(0);

  struct itimerval it = {};
  it.it_value.tv_sec = 0;
  it.it_value.tv_usec = 1000000 / TIMER_HZ;
  it.it_interval = it.it_value;
  int ret = setitimer(ITIMER_VIRTUAL, &it, NULL);
  assert(ret == 0);
}

bool cte_init(Context*(*handler)(Event, Context*)) {
  user_handler = handler;

  install_signal_handler();
  __am_init_timer_irq();
  return true;
}

Context* kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *c = (Context*)kstack.end - 1;

  __am_get_example_uc(c);
  c->uc.uc_mcontext.gregs[REG_RIP] = (uintptr_t)__am_kcontext_start;
  c->uc.uc_mcontext.gregs[REG_RSP] = (uintptr_t)kstack.end;

  int ret = sigemptyset(&(c->uc.uc_sigmask)); // enable interrupt
  assert(ret == 0);

  c->vm_head = NULL;

  c->GPR1 = (uintptr_t)arg;
  c->GPR2 = (uintptr_t)entry;
  return c;
}

void yield() {
  raise(SIGUSR2);
}

bool ienabled() {
  sigset_t set;
  int ret = sigprocmask(0, NULL, &set);
  assert(ret == 0);
  return __am_is_sigmask_sti(&set);
}

void iset(bool enable) {
  extern sigset_t __am_intr_sigmask;
  // NOTE: sigprocmask does not supported in multithreading
  int ret = sigprocmask(enable ? SIG_UNBLOCK : SIG_BLOCK, &__am_intr_sigmask, NULL);
  assert(ret == 0);
}
