#include "kernel/types.h"
#include "user/user.h"

#define PHILOSOPHERS 5
#define CYCLES 5

struct shared {
  int completed[PHILOSOPHERS];
};

static void
philosopher(struct shared *s, int forks[PHILOSOPHERS], int id)
{
  int left = id;
  int right = (id + 1) % PHILOSOPHERS;
  int low = left < right ? left : right;
  int high = left < right ? right : left;

  for (int cycle = 0; cycle < CYCLES; cycle++) {
    printf("Philosopher %d: THINKING (cycle %d)\n", id, cycle + 1);
    pause(1);
    printf("Philosopher %d: HUNGRY\n", id);
    sem_wait(forks[low]);
    sem_wait(forks[high]);
    printf("Philosopher %d: EATING\n", id);
    pause(2);
    sem_post(forks[high]);
    sem_post(forks[low]);
  }
  s->completed[id] = CYCLES;
  printf("Philosopher %d: completed %d cycles\n", id, CYCLES);
}

int
main(void)
{
  struct shared *s = (struct shared *)shm_get();
  int forks[PHILOSOPHERS];
  if (s == 0) {
    printf("Dining: shared memory setup failed\n");
    exit(1);
  }
  for (int i = 0; i < PHILOSOPHERS; i++) {
    s->completed[i] = 0;
    forks[i] = sem_create(1);
    if (forks[i] < 0) {
      printf("Dining: semaphore setup failed\n");
      exit(1);
    }
  }

  for (int i = 0; i < PHILOSOPHERS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("Dining: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      philosopher(s, forks, i);
      exit(0);
    }
  }
  for (int i = 0; i < PHILOSOPHERS; i++)
    wait(0);
  int ok = 1;
  for (int i = 0; i < PHILOSOPHERS; i++)
    if (s->completed[i] != CYCLES)
      ok = 0;
  printf("Dining: all philosophers completed=%s\n", ok ? "YES" : "NO");
  for (int i = 0; i < PHILOSOPHERS; i++)
    sem_destroy(forks[i]);
  exit(ok ? 0 : 1);
}
