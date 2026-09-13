# Ajay Chaudhary (2401MC47)

# Operating Systems Lab Assignment 6

This submission implements deadlock avoidance, deadlock detection, deadlock
prevention, and synchronized resource sharing in user-space xv6-RISC-V
programs.

## Question 1: Banker's Algorithm Simulation

### Objective

Implement the Banker's Algorithm for deadlock avoidance.

### Implementation

The program in `Q1/bankers.c` models five processes and three resource types
using the following matrices and vector:

- `Allocation[5][3]`
- `Max[5][3]`
- `Available[3]`
- `Need[5][3]`

`Need` is calculated at runtime using:

`Need[i][j] = Max[i][j] - Allocation[i][j]`

The safety algorithm determines whether the current state is safe and prints a
valid safe sequence. The resource-request algorithm checks the request against
Need and Available, temporarily grants the request, reruns the safety
algorithm, commits safe allocations, and rolls back unsafe allocations.

### Test Results

The output in `Q1/Q1_output.txt` shows:

- Initial state: `SAFE`
- Safe sequence: `P1 -> P3 -> P4 -> P0 -> P2`
- A safe request from `P1` being granted
- An unsafe request from `P4` being denied
- Temporary allocation being rolled back

### Deliverables

- `Q1/bankers.c`
- `Q1/Q1_output.txt`

## Question 2: Deadlock Detection Using a Resource Allocation Graph

### Objective

Detect deadlocks by constructing a wait-for graph from allocation and request
matrices..

### Implementation

The program in `Q2/deadlockdetect.c` represents four processes and three
resource types. An edge `Pi -> Pj` is added when process `Pi` requests a
resource currently allocated to process `Pj`.

Depth-first search is used to detect cycles. When a cycle is found, the
processes in the exact cycle are printed.

### Test Results

The output in `Q2/Q2_output.txt` contains:

- An acyclic scenario reported as `NO DEADLOCK`
- A deadlocked scenario reported as `DEADLOCK DETECTED`
- The exact cycle:

`P0 -> P1 -> P2 -> P0`

### Deliverables

- `Q2/deadlockdetect.c`
- `Q2/Q2_output.txt`

## Question 3: Deadlock Prevention Using Resource Ordering

### Objective

Demonstrate deadlock between real xv6 processes and prevent it using a global
resource-ordering rule.

### Implementation

The program in `Q3/resourceorder.c` creates two real xv6 child processes and
two shared semaphore resources.

In the bad configuration:

- Process A acquires Lock1 and waits for Lock2.
- Process B acquires Lock2 and waits for Lock1.

This creates a circular wait and intentionally leaves both children blocked.

In the fixed configuration, both processes acquire resources in the same
global order:

`Lock1 -> Lock2`

They then release the resources in reverse order and terminate successfully.

### Test Results

`Q3/Q3_bad_output.txt` shows both processes acquiring one lock and then waiting
for the lock held by the other process. The intentional hang was terminated
using a host-side timeout.

`Q3/Q3_fixed_output.txt` shows:

`Both processes completed: YES`

### Deliverables

- `Q3/resourceorder.c`
- `Q3/Q3_bad_output.txt`
- `Q3/Q3_fixed_output.txt`
- `Q3/Q3_explanation.txt`

## Question 4: Combined Synchronization and Deadlock Avoidance

### Objective

Use real xv6 processes and synchronization primitives to share limited
resources without deadlock.

### Implementation

The program in `Q4/syncdeadlock.c` creates five forked processes and three
resource types:

| Resource | Available instances |
|---|---:|
| Printer | 2 |
| Scanner | 1 |
| Disk | 2 |

Each process requires two resource types. Kernel-backed semaphores protect the
resource pools, and shared memory records completion status.

All processes acquire resources according to the global order:

`Printer < Scanner < Disk`

Each process performs three cycles and prints resource requests, grants, work,
releases, and completion status. The ordering rule prevents circular wait.

### Test Results

The output in `Q4/Q4_output.txt` shows all five processes completing three
cycles:

- `P0 completed 3 cycles`
- `P1 completed 3 cycles`
- `P2 completed 3 cycles`
- `P3 completed 3 cycles`
- `P4 completed 3 cycles`

Final result:

`syncdeadlock: all processes completed=YES`

### Deliverables

- `Q4/syncdeadlock.c`
- `Q4/Q4_output.txt`
- `Q4/Q4_explanation.txt`

 
