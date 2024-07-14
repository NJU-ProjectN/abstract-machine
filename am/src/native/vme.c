#define _GNU_SOURCE
#include <search.h>
#include "platform.h"

#define USER_SPACE RANGE(0x40000000, 0xc0000000)

typedef struct PageMap {
  void *va;
  void *pa;
  struct PageMap *next;
  int prot;
  int is_mapped;
  char key[32];  // used for hsearch_r()
} PageMap;

typedef struct VMHead {
  PageMap *head;
  struct hsearch_data hash;
  int nr_page;
} VMHead;

#define list_foreach(p, head) \
  for (p = (PageMap *)(head); p != NULL; p = p->next)

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
  VMHead *h = pgalloc(__am_pgsize); // used as head of the list
  assert(h != NULL);
  memset(h, 0, sizeof(*h));
  int max_pg = (USER_SPACE.end - USER_SPACE.start) / __am_pgsize;
  int ret = hcreate_r(max_pg, &h->hash);
  assert(ret != 0);

  as->ptr = h;
  as->pgsize = __am_pgsize;
  as->area = USER_SPACE;
}

void unprotect(AddrSpace *as) {
}

void __am_switch(Context *c) {
  if (!vme_enable) return;

  VMHead *head = c->vm_head;
  VMHead *now_head = thiscpu->vm_head;
  if (head == now_head) goto end;

  PageMap *pp;
  if (now_head != NULL) {
    // munmap all mappings
    list_foreach(pp, now_head->head) {
      if (pp->is_mapped) {
        __am_pmem_unmap(pp->va);
        pp->is_mapped = false;
      }
    }
  }

  if (head != NULL) {
    // mmap all mappings
    list_foreach(pp, head->head) {
      assert(IN_RANGE(pp->va, USER_SPACE));
      __am_pmem_map(pp->va, pp->pa, pp->prot);
      pp->is_mapped = true;
    }
  }

end:
  thiscpu->vm_head = head;
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  assert(IN_RANGE(va, USER_SPACE));
  assert((uintptr_t)va % __am_pgsize == 0);
  assert((uintptr_t)pa % __am_pgsize == 0);
  assert(as != NULL);
  PageMap *pp = NULL;
  VMHead *vm_head = as->ptr;
  assert(vm_head != NULL);
  char buf[32];
  snprintf(buf, 32, "%x", va);
  ENTRY item = { .key = buf };
  ENTRY *item_find;
  hsearch_r(item, FIND, &item_find, &vm_head->hash);
  if (item_find == NULL) {
    pp = pgalloc(__am_pgsize); // this will waste memory, any better idea?
    snprintf(pp->key, 32, "%x", va);
    item.key = pp->key;
    item.data = pp;
    int ret = hsearch_r(item, ENTER, &item_find, &vm_head->hash);
    assert(ret != 0);
    vm_head->nr_page ++;
  } else {
    pp = item_find->data;
  }
  pp->va = va;
  pp->pa = pa;
  pp->prot = prot;
  pp->is_mapped = false;
  pp->next = vm_head->head;
  vm_head->head = pp;

  if (vm_head == thiscpu->vm_head) {
    // enforce the map immediately
    __am_pmem_map(pp->va, pp->pa, pp->prot);
    pp->is_mapped = true;
  }
}

Context* ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *c = (Context*)kstack.end - 1;

  __am_get_example_uc(c);
  AM_REG_PC(&c->uc) = (uintptr_t)entry;
  AM_REG_SP(&c->uc) = (uintptr_t)USER_SPACE.end;

  int ret = sigemptyset(&(c->uc.uc_sigmask)); // enable interrupt
  assert(ret == 0);
  c->vm_head = as->ptr;

  c->ksp = (uintptr_t)kstack.end;

  return c;
}

int __am_in_userspace(void *addr) {
  return vme_enable && thiscpu->vm_head != NULL && IN_RANGE(addr, USER_SPACE);
}
