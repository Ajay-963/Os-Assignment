#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define NSEMS 64

struct {
  struct spinlock lock;
  uint64 pa;
  int refs;
} shared_page;

struct semaphore {
  int used;
  int value;
};

struct {
  struct spinlock lock;
  struct semaphore sem[NSEMS];
} semtab;

void
syncinit(void)
{
  initlock(&shared_page.lock, "shared_page");
  initlock(&semtab.lock, "semtab");
}

uint64
shm_get(void)
{
  struct proc *p = myproc();

  if (p->shm_pa != 0)
    return SHMBASE;
  acquire(&shared_page.lock);
  if (shared_page.pa == 0) {
    shared_page.pa = (uint64)kalloc();
    if (shared_page.pa == 0) {
      release(&shared_page.lock);
      return 0;
    }
    memset((void *)shared_page.pa, 0, PGSIZE);
  }
  if (mappages(p->pagetable, SHMBASE, PGSIZE, shared_page.pa,
               PTE_R | PTE_W | PTE_U) < 0) {
    release(&shared_page.lock);
    return 0;
  }
  shared_page.refs++;
  p->shm_pa = shared_page.pa;
  release(&shared_page.lock);
  return SHMBASE;
}

int
shm_fork(struct proc *parent, struct proc *child)
{
  if (parent->shm_pa == 0)
    return 0;

  acquire(&shared_page.lock);
  if (shared_page.pa != parent->shm_pa || shared_page.refs <= 0) {
    release(&shared_page.lock);
    return -1;
  }
  if (mappages(child->pagetable, SHMBASE, PGSIZE, parent->shm_pa,
               PTE_R | PTE_W | PTE_U) < 0) {
    release(&shared_page.lock);
    return -1;
  }
  child->shm_pa = parent->shm_pa;
  shared_page.refs++;
  release(&shared_page.lock);
  return 0;
}

void
shm_release(struct proc *p)
{
  if (p->shm_pa == 0)
    return;

  uvmunmap(p->pagetable, SHMBASE, 1, 0);
  acquire(&shared_page.lock);
  if (shared_page.pa == p->shm_pa && shared_page.refs > 0) {
    shared_page.refs--;
    if (shared_page.refs == 0) {
      kfree((void *)shared_page.pa);
      shared_page.pa = 0;
    }
  }
  release(&shared_page.lock);
  p->shm_pa = 0;
}

int
sem_create(int value)
{
  int id = -1;
  if (value < 0)
    return -1;
  acquire(&semtab.lock);
  for (int i = 0; i < NSEMS; i++) {
    if (!semtab.sem[i].used) {
      semtab.sem[i].used = 1;
      semtab.sem[i].value = value;
      id = i;
      break;
    }
  }
  release(&semtab.lock);
  return id;
}

int
sem_wait(int id)
{
  if (id < 0 || id >= NSEMS)
    return -1;
  acquire(&semtab.lock);
  for (;;) {
    if (!semtab.sem[id].used) {
      release(&semtab.lock);
      return -1;
    }
    if (semtab.sem[id].value > 0) {
      semtab.sem[id].value--;
      release(&semtab.lock);
      return 0;
    }
    sleep_prepare(&semtab.sem[id]);
    release(&semtab.lock);
    sleep();
    acquire(&semtab.lock);
  }
}

int
sem_post(int id)
{
  if (id < 0 || id >= NSEMS)
    return -1;
  acquire(&semtab.lock);
  if (!semtab.sem[id].used) {
    release(&semtab.lock);
    return -1;
  }
  semtab.sem[id].value++;
  wakeup(&semtab.sem[id]);
  release(&semtab.lock);
  return 0;
}

int
sem_destroy(int id)
{
  if (id < 0 || id >= NSEMS)
    return -1;
  acquire(&semtab.lock);
  if (!semtab.sem[id].used) {
    release(&semtab.lock);
    return -1;
  }
  semtab.sem[id].used = 0;
  semtab.sem[id].value = 0;
  wakeup(&semtab.sem[id]);
  release(&semtab.lock);
  return 0;
}
