#include "kernel/types.h"
#include "user/user.h"

struct shared {
  int bad_ready;
  int fixed_done[2];
};

static void
fixed_process(struct shared *s, int id, int lock1, int lock2)
{
  printf("[fixed] Process %d requesting Lock1\n", id);
  sem_wait(lock1);
  printf("[fixed] Process %d acquired Lock1\n", id);
  pause(1);
  printf("[fixed] Process %d requesting Lock2\n", id);
  sem_wait(lock2);
  printf("[fixed] Process %d acquired Lock2\n", id);
  printf("[fixed] Process %d completed work\n", id);
  sem_post(lock2);
  sem_post(lock1);
  printf("[fixed] Process %d released Lock2 and Lock1\n", id);
  s->fixed_done[id] = 1;
  exit(0);
}

static int
run_bad(void)
{
  struct shared *s = (struct shared *)shm_get();
  int lock1 = sem_create(1);
  int lock2 = sem_create(1);
  if (s == 0 || lock1 < 0 || lock2 < 0) {
    printf("[bad] setup failed\n");
    exit(1);
  }
  printf("[bad] Starting intentional circular wait; stop xv6 with Ctrl-A X or timeout\n");
  int pid = fork();
  if (pid < 0)
    exit(1);
  if (pid == 0) {
    sem_wait(lock1);
    printf("[bad] Process A acquired Lock1\n");
    pause(3);
    printf("[bad] Process A waiting for Lock2\n");
    sem_wait(lock2);
    exit(0);
  }
  pause(1);
  pid = fork();
  if (pid < 0)
    exit(1);
  if (pid == 0) {
    sem_wait(lock2);
    printf("[bad] Process B acquired Lock2\n");
    pause(3);
    printf("[bad] Process B waiting for Lock1\n");
    sem_wait(lock1);
    exit(0);
  }
  wait(0);
  wait(0);
  sem_destroy(lock1);
  sem_destroy(lock2);
  return 0;
}

static int
run_fixed(void)
{
  struct shared *s = (struct shared *)shm_get();
  int lock1 = sem_create(1);
  int lock2 = sem_create(1);
  if (s == 0 || lock1 < 0 || lock2 < 0) {
    printf("[fixed] setup failed\n");
    exit(1);
  }
  s->fixed_done[0] = 0;
  s->fixed_done[1] = 0;
  for (int i = 0; i < 2; i++) {
    int pid = fork();
    if (pid < 0)
      exit(1);
    if (pid == 0)
      fixed_process(s, i, lock1, lock2);
  }
  wait(0);
  wait(0);
  int ok = s->fixed_done[0] && s->fixed_done[1];
  sem_destroy(lock1);
  sem_destroy(lock2);
  printf("[fixed] Both processes completed: %s\n", ok ? "YES" : "NO");
  return ok ? 0 : 1;
}

int
main(int argc, char **argv)
{
  if (argc > 1 && argv[1][0] == 'b')
    exit(run_bad());
  if (argc > 1 && argv[1][0] == 'f')
    exit(run_fixed());
  printf("usage: resourceorder bad | fixed\n");
  exit(1);
}
