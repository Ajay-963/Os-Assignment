#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;
extern char trampoline[];
extern void forkret(void);
extern void usertrap(void);

#define MAX_SEMAPHORES 64

struct sem_entry {
  int value;
  int in_use;
  struct spinlock lock;
};

static struct sem_entry semaphore_table[MAX_SEMAPHORES];
static struct spinlock sem_table_lock;

void
sysprocinit(void)
{
  initlock(&sem_table_lock, "sem_table_lock");
  for (int i = 0; i < MAX_SEMAPHORES; i++) {
    initlock(&semaphore_table[i].lock, "sem");
  }
}

uint64
sys_clone(void)
{
  uint64 fn, arg, stack;
  struct proc *p = myproc();
  struct proc *np;

  argaddr(0, &fn);
  argaddr(1, &arg);
  argaddr(2, &stack);

  for (np = proc; np < &proc[NPROC]; np++) {
    acquire(&np->lock);
    if (np->state == UNUSED) {
      goto found;
    }
    release(&np->lock);
  }
  return -1;

found:
  np->pid = allocpid();
  np->state = USED;
  np->is_thread = 1;
  np->trapframe_mapped = 0;
  np->pagetable = p->pagetable;
  np->sz = p->sz;
  np->priority = p->priority;
  np->min_priority = p->priority;
  np->parent = p;
  np->killed = 0;
  np->xstate = 0;

  if ((np->trapframe = (struct trapframe *)kalloc()) == 0) {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  memset(np->trapframe, 0, PGSIZE);
  if (mappages(np->pagetable, np->trapframe_va, PGSIZE,
               (uint64)np->trapframe, PTE_R | PTE_W) < 0) {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->trapframe_mapped = 1;

  for (int i = 0; i < NOFILE; i++) {
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  }
  np->cwd = idup(p->cwd);
  safestrcpy(np->name, p->name, sizeof(p->name));

  memset(&np->context, 0, sizeof(np->context));
  np->context.ra = (uint64)forkret;
  np->context.sp = np->kstack + PGSIZE;

  *(np->trapframe) = *(p->trapframe);
  np->trapframe->kernel_satp = r_satp();
  np->trapframe->kernel_sp = np->kstack + PGSIZE;
  np->trapframe->kernel_trap = (uint64)usertrap;
  np->trapframe->kernel_hartid = r_tp();
  np->trapframe->epc = fn;
  np->trapframe->sp = stack;
  np->trapframe->a0 = arg;
  np->trapframe->ra = 0;
  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return np->pid;
}

uint64
sys_join(void)
{
  struct proc *pp;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;) {
    int have_threads = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++) {
      if (pp->parent == p && pp->is_thread) {
        acquire(&pp->lock);
        have_threads = 1;
        if (pp->state == ZOMBIE) {
          int pid = pp->pid;
          pp->parent = 0;
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    if (!have_threads || killed(p)) {
      release(&wait_lock);
      return -1;
    }

    sleep_prepare(p);
    release(&wait_lock);
    sleep();
    acquire(&wait_lock);
  }
}

uint64
sys_sem_init(void)
{
  int sem_id;
  int value;

  argint(0, &sem_id);
  argint(1, &value);
  if (sem_id < 0 || sem_id >= MAX_SEMAPHORES || value < 0)
    return -1;

  acquire(&sem_table_lock);
  acquire(&semaphore_table[sem_id].lock);
  semaphore_table[sem_id].value = value;
  semaphore_table[sem_id].in_use = 1;
  release(&semaphore_table[sem_id].lock);
  release(&sem_table_lock);
  return 0;
}

uint64
sys_sem_wait(void)
{
  int sem_id;
  argint(0, &sem_id);
  if (sem_id < 0 || sem_id >= MAX_SEMAPHORES)
    return -1;

  acquire(&sem_table_lock);
  if (!semaphore_table[sem_id].in_use) {
    release(&sem_table_lock);
    return -1;
  }
  release(&sem_table_lock);

  for (;;) {
    acquire(&semaphore_table[sem_id].lock);
    if (semaphore_table[sem_id].value > 0) {
      semaphore_table[sem_id].value--;
      release(&semaphore_table[sem_id].lock);
      return 0;
    }
    sleep_prepare(&semaphore_table[sem_id]);
    release(&semaphore_table[sem_id].lock);
    sleep();
  }
}

uint64
sys_sem_signal(void)
{
  int sem_id;
  argint(0, &sem_id);
  if (sem_id < 0 || sem_id >= MAX_SEMAPHORES)
    return -1;

  acquire(&sem_table_lock);
  if (!semaphore_table[sem_id].in_use) {
    release(&sem_table_lock);
    return -1;
  }
  release(&sem_table_lock);

  acquire(&semaphore_table[sem_id].lock);
  semaphore_table[sem_id].value++;
  wakeup(&semaphore_table[sem_id]);
  release(&semaphore_table[sem_id].lock);
  return 0;
}

uint64
sys_setpriority(void)
{
  int pid;
  int priority;

  argint(0, &pid);
  argint(1, &priority);
  return setpriority(pid, priority);
}

uint64
sys_getpriority(void)
{
  int pid;
  argint(0, &pid);
  return getpriority(pid);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > USER_TOP)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
