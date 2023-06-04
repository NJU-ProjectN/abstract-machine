#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/auxv.h>
#include <dlfcn.h>
#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"

#define MAX_CPU 16
#define TRAP_PAGE_START (void *)0x100000
#define PMEM_START (void *)0x1000000  // for nanos-lite with vme disabled
#define PMEM_SIZE (128 * 1024 * 1024) // 128MB
static int pmem_fd = 0;
static void *pmem = NULL;
static ucontext_t uc_example = {};
static void *(*memcpy_libc)(void *, const void *, size_t) = NULL;
sigset_t __am_intr_sigmask = {};
__am_cpu_t *__am_cpu_struct = NULL;
int __am_ncpu = 0;
int __am_pgsize = 0;

static void save_context_handler(int sig, siginfo_t *info, void *ucontext) {
  memcpy_libc(&uc_example, ucontext, sizeof(uc_example));
}

static void save_example_context() {
  // getcontext() does not save segment registers. In the signal
  // handler, restoring a context previously saved by getcontext()
  // will trigger segmentation fault because of the invalid segment
  // registers. So we save the example context during signal handling
  // to get a context with everything valid.
  struct sigaction s;
  void *(*memset_libc)(void *, int, size_t) = dlsym(RTLD_NEXT, "memset");
  memset_libc(&s, 0, sizeof(s));
  s.sa_sigaction = save_context_handler;
  s.sa_flags = SA_SIGINFO;
  int ret = sigaction(SIGUSR1, &s, NULL);
  assert(ret == 0);

  raise(SIGUSR1);

  s.sa_flags = 0;
  s.sa_handler = SIG_DFL;
  ret = sigaction(SIGUSR1, &s, NULL);
  assert(ret == 0);
}

static void setup_sigaltstack() {
  assert(sizeof(thiscpu->sigstack) >= SIGSTKSZ);
  stack_t ss;
  ss.ss_sp = thiscpu->sigstack;
  ss.ss_size = sizeof(thiscpu->sigstack);
  ss.ss_flags = 0;
  int ret = sigaltstack(&ss, NULL);
  assert(ret == 0);
}

int main(const char *args);

static void init_platform() __attribute__((constructor));
static void init_platform() {
  // create memory object and set up mapping to simulate the physical memory
  pmem_fd = memfd_create("pmem", 0);
  assert(pmem_fd != -1);
  // use dynamic linking to avoid linking to the same function in RT-Thread
  int (*ftruncate_libc)(int, off_t) = dlsym(RTLD_NEXT, "ftruncate");
  assert(ftruncate_libc != NULL);
  int ret2 = ftruncate_libc(pmem_fd, PMEM_SIZE);
  assert(ret2 == 0);

  pmem = mmap(PMEM_START, PMEM_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_SHARED | MAP_FIXED, pmem_fd, 0);
  assert(pmem != (void *)-1);

  // allocate private per-cpu structure
  thiscpu = mmap(NULL, sizeof(*thiscpu), PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(thiscpu != (void *)-1);
  thiscpu->cpuid = 0;
  thiscpu->vm_head = NULL;

  // create trap page to receive syscall and yield by SIGSEGV
  int sys_pgsz = sysconf(_SC_PAGESIZE);
  void *ret = mmap(TRAP_PAGE_START, sys_pgsz, PROT_NONE,
      MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  assert(ret != (void *)-1);

  // save the address of memcpy() in glibc, since it may be linked with klib
  memcpy_libc = dlsym(RTLD_NEXT, "memcpy");
  assert(memcpy_libc != NULL);

  // remap writable sections as MAP_SHARED
  Elf64_Phdr *phdr = (void *)getauxval(AT_PHDR);
  int phnum = (int)getauxval(AT_PHNUM);
  int i;
  for (i = 0; i < phnum; i ++) {
    if (phdr[i].p_type == PT_LOAD && (phdr[i].p_flags & PF_W)) {
      // allocate temporary memory
      extern char end;
      void *vaddr = (void *)&end - phdr[i].p_memsz;
      uintptr_t pad = (uintptr_t)vaddr & 0xfff;
      void *vaddr_align = vaddr - pad;
      uintptr_t size = phdr[i].p_memsz + pad;
      void *temp_mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      assert(temp_mem != (void *)-1);

      // save data and bss sections
      memcpy_libc(temp_mem, vaddr_align, size);

      // save the address of mmap() which will be used after munamp(),
      // since calling the library functions requires accessing GOT, which will be unmapped
      void *(*mmap_libc)(void *, size_t, int, int, int, off_t) = dlsym(RTLD_NEXT, "mmap");
      assert(mmap_libc != NULL);
      // load the address of memcpy() on stack, which can still be accessed
      // after the data section is unmapped
      void *(*volatile memcpy_libc_temp)(void *, const void *, size_t) = memcpy_libc;

      // unmap the data and bss sections
      ret2 = munmap(vaddr_align, size);
      assert(ret2 == 0);

      // map the sections again with MAP_SHARED, which will be shared across fork()
      ret = mmap_libc(vaddr_align, size, PROT_READ | PROT_WRITE | PROT_EXEC,
          MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
      assert(ret == vaddr_align);

      // restore the data in the sections
      memcpy_libc_temp(vaddr_align, temp_mem, size);

      // unmap the temporary memory
      ret2 = munmap(temp_mem, size);
      assert(ret2 == 0);
    }
  }

  // set up the AM heap
  heap = RANGE(pmem, pmem + PMEM_SIZE);

  // initialize sigmask for interrupts
  ret2 = sigemptyset(&__am_intr_sigmask);
  assert(ret2 == 0);
  ret2 = sigaddset(&__am_intr_sigmask, SIGVTALRM);
  assert(ret2 == 0);
  ret2 = sigaddset(&__am_intr_sigmask, SIGUSR1);
  assert(ret2 == 0);

  // setup alternative signal stack
  setup_sigaltstack();

  // save the context template
  save_example_context();
  uc_example.uc_mcontext.fpregs = NULL; // clear the FPU context
  __am_get_intr_sigmask(&uc_example.uc_sigmask);

  // disable interrupts by default
  iset(0);

  // set ncpu
  const char *smp = getenv("smp");
  __am_ncpu = smp ? atoi(smp) : 1;
  assert(0 < __am_ncpu && __am_ncpu <= MAX_CPU);

  // set pgsize
  const char *pgsize = getenv("pgsize");
  __am_pgsize = pgsize ? atoi(pgsize) : sys_pgsz;
  assert(__am_pgsize > 0 && __am_pgsize % sys_pgsz == 0);

  // set stdout unbuffered
  setbuf(stdout, NULL);

  const char *args = getenv("mainargs");
  halt(main(args ? args : "")); // call main here!
}

void __am_exit_platform(int code) {
  // let Linux clean up other resource
  extern int __am_mpe_init;
  if (__am_mpe_init && cpu_count() > 1) kill(0, SIGKILL);
  exit(code);
}

void __am_pmem_map(void *va, void *pa, int prot) {
  // translate AM prot to mmap prot
  int mmap_prot = PROT_NONE;
  // we do not support executable bit, so mark
  // all readable pages executable as well
  if (prot & MMAP_READ) mmap_prot |= PROT_READ | PROT_EXEC;
  if (prot & MMAP_WRITE) mmap_prot |= PROT_WRITE;
  void *ret = mmap(va, __am_pgsize, mmap_prot,
      MAP_SHARED | MAP_FIXED, pmem_fd, (uintptr_t)(pa - pmem));
  assert(ret != (void *)-1);
}

void __am_pmem_unmap(void *va) {
  int ret = munmap(va, __am_pgsize);
  assert(ret == 0);
}

void __am_get_example_uc(Context *r) {
  memcpy_libc(&r->uc, &uc_example, sizeof(uc_example));
}

void __am_get_intr_sigmask(sigset_t *s) {
  memcpy_libc(s, &__am_intr_sigmask, sizeof(__am_intr_sigmask));
}

int __am_is_sigmask_sti(sigset_t *s) {
  return !sigismember(s, SIGVTALRM);
}

void __am_send_kbd_intr() {
  kill(getpid(), SIGUSR1);
}

void __am_pmem_protect() {
//  int ret = mprotect(PMEM_START, PMEM_SIZE, PROT_NONE);
//  assert(ret == 0);
}

void __am_pmem_unprotect() {
//  int ret = mprotect(PMEM_START, PMEM_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
//  assert(ret == 0);
}

// This dummy function will be called in trm.c.
// The purpose of this dummy function is to let linker add this file to the object
// file set. Without it, the constructor of @_init_platform will not be linked.
void __am_platform_dummy() {
}
