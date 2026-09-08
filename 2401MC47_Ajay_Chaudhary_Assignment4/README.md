## [Ajay Chaudhary - 2401MC47]
 
# OS Lab Assignment 5
## Process Synchronization using xv6

## Environment

This submission targets the existing `xv6-riscv` tree at
`/home/ajay/os-lab/xv6-riscv`, using the RISC-V GCC toolchain and QEMU.
The implementation was tested with the tree's existing three-CPU QEMU
configuration.

## Build and run

```sh
cd /home/ajay/os-lab/xv6-riscv
make clean
make fs.img
make qemu
```

At the xv6 shell, run `peterson`, `prodcons`, `readwrite`, or `dining`.
Producer-consumer accepts an optional capacity, for example `prodcons 3`.
To capture a run, redirect the QEMU terminal output from the host terminal:

```sh
(sleep 2; printf 'peterson\n'; sleep 8) | timeout 20 make qemu > /home/ajay/os-lab/Assignment5/output_logs/peterson.txt 2>&1
```

Use the same command with `prodcons`, `readwrite`, and `dining` and a suitable
timeout for the other logs. The existing output logs in this submission are
from actual QEMU runs. After reviewing the folder, archive it with:

```sh
cd /home/ajay/os-lab
tar -czf RollNumber_Name_Assignment4.tar.gz Assignment5
```

## Kernel support

`kernel/sync.c` adds:

- A fixed shared page at `SHMBASE`. `shm_get()` allocates and maps one physical
  page for a process. `kfork()` maps that same physical page into the child and
  maintains a reference count; process cleanup unmaps it and frees it only
  after the last process exits.
- A table of 64 kernel-backed semaphores. `sem_wait()` decrements an available
  count or sleeps on the semaphore channel; `sem_post()` increments and wakes
  waiters. The table spinlock protects semaphore state.

The new calls are `shm_get`, `sem_create`, `sem_wait`, `sem_post`, and
`sem_destroy`, wired through `syscall.h`, `syscall.c`, `sysproc.c`,
`user/usys.pl`, and `user/user.h`.

## Question 1: Peterson

`user/peterson.c` uses the shared page for `flag[2]`, `turn`, and
`shared_counter`. The parent and child each execute ten iterations and use
only Peterson's `flag[]` and `turn` variables for the critical section. The
counter reaches 20. The shared structure is volatile so compiler optimization
does not remove the required busy-wait loads.

## Question 2: Producer-consumer

`user/prodcons.c` implements a circular buffer (default capacity 5), one
producer, and one consumer. The shared page contains the buffer and indices.
The kernel semaphores are `empty`, `full`, and `mutex`; the producer waits on
`empty`, inserts under `mutex`, then signals `full`, while the consumer uses
the inverse order. Values 1 through 20 are verified for exact ordered
consumption.

## Question 3: Readers-writers

`user/readwrite.c` creates three readers and two writers. `rmutex` protects
`read_count`; the first reader acquires `resource`, and the last reader
releases it. Writers acquire `resource` exclusively. A `turnstile` semaphore
prevents a continuous stream of new readers from passing writers indefinitely,
so this is a fair admission variant of the classic readers-priority solution.

## Question 4: Dining philosophers

`user/dining.c` creates five philosopher processes and five binary fork
semaphores. Each philosopher reports THINKING, HUNGRY, and EATING for five
cycles. Forks are always acquired in increasing fork-number order, eliminating
circular wait and therefore deadlock. The parent verifies that all five
philosophers complete.

## Files changed

Relevant modified kernel files are included in `kernel/`: `defs.h`,
`main.c`, `memlayout.h`, `proc.c`, `proc.h`, `syscall.c`, `syscall.h`, and
`sysproc.c`, plus the new `sync.c`. The updated top-level `Makefile` and
`user/user.h`/`user/usys.pl` are included. The four new user programs are in
`user/`.

## Testing and known limitations

Each program was run under QEMU and completed without a kernel panic or
permanent synchronization hang. QEMU is intentionally terminated by the host
`timeout` command after the program returns to the xv6 shell; this is why the
captured logs end with QEMU's signal-15 shutdown line. Output characters from
different processes can interleave on the xv6 console, but the shared-state
checks and completion summaries are genuine.
