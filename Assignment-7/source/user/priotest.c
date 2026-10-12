#include "kernel/types.h"
#include "user/user.h"

#define PRIORITIES 4
#define WORK_PER_CHILD 900000000
#define AGING_POLL_TICKS 100

struct completion {
  int pid;
  int priority;
  int elapsed;
  int effective_priority;
};

static void
run_workload(int priority, int fd, int experiment_start)
{
  volatile uint value = 0;

  for (int i = 0; i < WORK_PER_CHILD; i++)
    value += (uint)i ^ (uint)priority;

  struct completion result = {
    getpid(), priority, uptime() - experiment_start, getpriority(getpid())
  };
  if (write(fd, &result, sizeof(result)) != sizeof(result))
    exit(1);
  (void)value;
  exit(0);
}

int
main(void)
{
  int priorities[PRIORITIES] = {1, 5, 10, 20};
  int fds[2];
  int pids[PRIORITIES];
  int lowest_priority = 20;
  int experiment_start;

  if (pipe(fds) < 0) {
    printf("Q4 FAIL: pipe\n");
    exit(1);
  }
  experiment_start = uptime();

  for (int i = 0; i < PRIORITIES; i++) {
    int old_priority = setpriority(getpid(), priorities[i]);
    if (old_priority < 0) {
      printf("Q4 FAIL: set parent priority\n");
      exit(1);
    }
    pids[i] = fork();
    if (pids[i] < 0) {
      printf("Q4 FAIL: fork\n");
      exit(1);
    }
    if (pids[i] == 0) {
      close(fds[0]);
      run_workload(priorities[i], fds[1], experiment_start);
    }
    if (setpriority(getpid(), old_priority) < 0) {
      printf("Q4 FAIL: restore parent priority\n");
      exit(1);
    }
    printf("Q4 started pid=%d initial_priority=%d\n", pids[i], priorities[i]);
  }
  close(fds[1]);

  int aged = 0;
  for (int tick = 0; tick < AGING_POLL_TICKS; tick++) {
    pause(1);
    int effective = getpriority(pids[PRIORITIES - 1]);
    if (effective >= 0 && effective < lowest_priority) {
      lowest_priority = effective;
      aged = 1;
      printf("Q4 aging observed pid=%d initial_priority=20 effective_priority=%d\n",
             pids[PRIORITIES - 1], effective);
    }
  }

  int completion_order = 0;
  for (int i = 0; i < PRIORITIES; i++) {
    struct completion result;
    int n = read(fds[0], &result, sizeof(result));
    if (n != sizeof(result)) {
      printf("Q4 FAIL: completion read\n");
      exit(1);
    }
    completion_order++;
    printf("Q4 completion order=%d pid=%d initial_priority=%d elapsed_ticks=%d effective_priority=%d\n",
           completion_order, result.pid, result.priority, result.elapsed,
           result.effective_priority);
  }
  close(fds[0]);

  for (int i = 0; i < PRIORITIES; i++) {
    if (wait(0) < 0) {
      printf("Q4 FAIL: wait\n");
      exit(1);
    }
  }
  if (!aged || lowest_priority >= 20) {
    printf("Q4 FAIL: aging not observed for priority 20\n");
    exit(1);
  }
  printf("Q4 PASS: priority 20 reached effective %d\n", lowest_priority);
  exit(0);
}
