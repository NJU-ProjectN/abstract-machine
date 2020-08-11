#include "x86-qemu.h"

const struct mmu_config mmu = {
  .pgsize = 4096,
#if __x86_64__
  .ptlevels = 4,
  .pgtables = {
    { "CR3",  0x000000000000,  0,  0 },
    { "PML4", 0xff8000000000, 39,  9 },
    { "PDPT", 0x007fc0000000, 30,  9 },
    { "PD",   0x00003fe00000, 21,  9 },
    { "PT",   0x0000001ff000, 12,  9 },
  },
#else
  .ptlevels = 2,
  .pgtables = {
    { "CR3",      0x00000000,  0,  0 },
    { "PD",       0xffc00000, 22, 10 },
    { "PT",       0x003ff000, 12, 10 },
  },
#endif
};

static const struct vm_area vm_areas[] = {
#ifdef __x86_64__
  { RANGE(0x100000000000, 0x108000000000), 0 }, // 512 GiB user space
  { RANGE(0x000000000000, 0x008000000000), 1 }, // 512 GiB kernel
#else
  { RANGE(    0x40000000,     0x80000000), 0 }, // 1 GiB user space
  { RANGE(    0x00000000,     0x40000000), 1 }, // 1 GiB kernel
  { RANGE(    0xfd000000,     0x00000000), 1 }, // memory-mapped I/O
#endif
};
#define uvm_area (vm_areas[0].area)

static uintptr_t *kpt;
static void *(*pgalloc)(int size);
static void (*pgfree)(void *);

static void *pgallocz() {
  uintptr_t *base = pgalloc(mmu.pgsize);
  panic_on(!base, "cannot allocate page");
  for (int i = 0; i < mmu.pgsize / sizeof(uintptr_t); i++) {
    base[i] = 0;
  }
  return base;
}

static int indexof(uintptr_t addr, const struct ptinfo *info) {
  return ((uintptr_t)addr & info->mask) >> info->shift;
}

static uintptr_t baseof(uintptr_t addr) {
  return addr & ~(mmu.pgsize - 1);
}

static uintptr_t *ptwalk(AddrSpace *as, uintptr_t addr, int flags) {
  uintptr_t cur = (uintptr_t)&as->ptr;

  for (int i = 0; i <= mmu.ptlevels; i++) {
    const struct ptinfo *ptinfo = &mmu.pgtables[i];
    uintptr_t *pt = (uintptr_t *)cur, next_page;
    int index = indexof(addr, ptinfo);
    if (i == mmu.ptlevels) return &pt[index];

    if (!(pt[index] & PTE_P)) {
      next_page = (uintptr_t)pgallocz();
      pt[index] = next_page | PTE_P | flags;
    } else {
      next_page = baseof(pt[index]);
    }
    cur = next_page;
  }
  bug();
}

static void teardown(int level, uintptr_t *pt) {
  if (level > mmu.ptlevels) return;
  for (int index = 0; index < (1 << mmu.pgtables[level].bits); index++) {
    if ((pt[index] & PTE_P) && (pt[index] & PTE_U)) {
      teardown(level + 1, (void *)baseof(pt[index]));
    }
  }
  if (level >= 1) {
    pgfree(pt);
  }
}

bool vme_init(void *(*_pgalloc)(int size), void (*_pgfree)(void *)) {
  panic_on(cpu_current() != 0, "init VME in non-bootstrap CPU");
  pgalloc = _pgalloc;
  pgfree  = _pgfree;

#if __x86_64__
  kpt = (void *)PML4_ADDR;
#else
  AddrSpace as;
  as.ptr = NULL;
  for (int i = 0; i < LENGTH(vm_areas); i++) {
    const struct vm_area *vma = &vm_areas[i];
    if (vma->kernel) {
      for (uintptr_t cur = (uintptr_t)vma->area.start;
           cur != (uintptr_t)vma->area.end;
           cur += mmu.pgsize) {
        *ptwalk(&as, cur, PTE_W) = cur | PTE_P | PTE_W;
      }
    }
  }
  kpt = (void *)baseof((uintptr_t)as.ptr);
#endif

  set_cr3(kpt);
  set_cr0(get_cr0() | CR0_PG);
  return true;
}

void protect(AddrSpace *as) {
  uintptr_t *upt = pgallocz();

  for (int i = 0; i < LENGTH(vm_areas); i++) {
    const struct vm_area *vma = &vm_areas[i];
    if (vma->kernel) {
      const struct ptinfo *info = &mmu.pgtables[1]; // level-1 page table
      for (uintptr_t cur = (uintptr_t)vma->area.start;
           cur != (uintptr_t)vma->area.end;
           cur += (1L << info->shift)) {
        int index = indexof(cur, info);
        upt[index] = kpt[index];
      }
    }
  }
  as->pgsize = mmu.pgsize;
  as->area   = uvm_area;
  as->ptr    = (void *)((uintptr_t)upt | PTE_P | PTE_U);
}

void unprotect(AddrSpace *as) {
  teardown(0, (void *)&as->ptr);
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  panic_on(!IN_RANGE(va, uvm_area), "mapping an invalid address");
  panic_on((uintptr_t)va != ROUNDDOWN(va, mmu.pgsize) ||
           (uintptr_t)pa != ROUNDDOWN(pa, mmu.pgsize), "non-page-boundary address");

  uintptr_t *ptentry = ptwalk(as, (uintptr_t)va, PTE_W | PTE_U);
  if (prot == MMAP_NONE) {
    panic_on(!(*ptentry & PTE_P), "unmapping a non-mapped page");
    *ptentry = 0;
  } else {
    panic_on(*ptentry & PTE_P, "remapping a mapped page");
    uintptr_t pte = (uintptr_t)pa | PTE_P | PTE_U | ((prot & MMAP_WRITE) ? PTE_W : 0);
    *ptentry = pte;
  }
  ptwalk(as, (uintptr_t)va, PTE_W | PTE_U);
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *ctx = kstack.end - sizeof(Context);
  *ctx = (Context) { 0 };

#if __x86_64__
  ctx->cs     = USEL(SEG_UCODE);
  ctx->ss     = USEL(SEG_UDATA);
  ctx->rip    = (uintptr_t)entry;
  ctx->rflags = FL_IF;
  ctx->rsp    = (uintptr_t)uvm_area.end;
  ctx->rsp0   = (uintptr_t)kstack.end;
#else
  ctx->cs     = USEL(SEG_UCODE);
  ctx->ds     = USEL(SEG_UDATA);
  ctx->ss3    = USEL(SEG_UDATA);
  ctx->eip    = (uintptr_t)entry;
  ctx->eflags = FL_IF;
  ctx->esp    = (uintptr_t)uvm_area.end;
  ctx->esp0   = (uintptr_t)kstack.end;
#endif
  ctx->cr3 = as->ptr;

  return ctx;
}
