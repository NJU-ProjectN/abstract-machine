#include "platform.h"

#define USER_SPACE RANGE(0x40000000, 0xc0000000)

typedef struct PageMap {
  void *va;
  void *pa;
  struct PageMap *next;
  int prot;
  int is_mapped;
} PageMap;

#define list_foreach(p, head) \
  for (p = ((PageMap *)(head))->next; p != NULL; p = p->next)

extern int __am_pgsize;
static int vme_enable = 0;
static void* (*pgalloc)(int) = NULL;
static void (*pgfree)(void *) = NULL;

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc = pgalloc_f;
  pgfree = pgfree_f;
  vme_enable = 1;
  return true;
}

void protect(AddrSpace *as) {
  assert(as != NULL);
  as->ptr = pgalloc(__am_pgsize); // used as head of the list
  as->pgsize = __am_pgsize;
  as->area = USER_SPACE;
}

void unprotect(AddrSpace *as) {
}

void __am_switch(Context *c) {
  if (!vme_enable) return;

  PageMap *head = c->vm_head;
  if (head == thiscpu->vm_head) return;

  PageMap *pp;
  if (thiscpu->vm_head != NULL) {
    // munmap all mappings
    list_foreach(pp, thiscpu->vm_head) {
      if (pp->is_mapped) {
        __am_pmem_unmap(pp->va);
        pp->is_mapped = false;
      }
    }
  }

  if (head != NULL) {
    // mmap all mappings
    list_foreach(pp, head) {
      assert(IN_RANGE(pp->va, USER_SPACE));
      __am_pmem_map(pp->va, pp->pa, pp->prot);
      pp->is_mapped = true;
    }
  }

  thiscpu->vm_head = head;
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  assert(IN_RANGE(va, USER_SPACE));
  assert((uintptr_t)va % __am_pgsize == 0);
  assert((uintptr_t)pa % __am_pgsize == 0);
  assert(as != NULL);
  PageMap *pp = NULL;
  PageMap *vm_head = as->ptr;
  assert(vm_head != NULL);
  list_foreach(pp, vm_head) {
    if (pp->va == va) break;
  }

  if (pp == NULL) {
    pp = pgalloc(__am_pgsize); // this will waste memory, any better idea?
  }
  pp->va = va;
  pp->pa = pa;
  pp->prot = prot;
  pp->is_mapped = false;
  // add after to vm_head to keep vm_head unchanged,
  // since vm_head is a key to describe an address space
  pp->next = vm_head->next;
  vm_head->next = pp;

  if (vm_head == thiscpu->vm_head) {
    // enforce the map immediately
    __am_pmem_map(pp->va, pp->pa, pp->prot);
    pp->is_mapped = true;
  }
}

Context* ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *c = (Context*)kstack.end - 1;

  __am_get_example_uc(c);
  c->uc.uc_mcontext.gregs[REG_RIP] = (uintptr_t)entry;
  c->uc.uc_mcontext.gregs[REG_RSP] = (uintptr_t)USER_SPACE.end;

  int ret = sigemptyset(&(c->uc.uc_sigmask)); // enable interrupt
  assert(ret == 0);
  c->vm_head = as->ptr;

  c->ksp = (uintptr_t)kstack.end;

  return c;
}

int __am_in_userspace(void *addr) {
  return vme_enable && thiscpu->vm_head != NULL && IN_RANGE(addr, USER_SPACE);
}
