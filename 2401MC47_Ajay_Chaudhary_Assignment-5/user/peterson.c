#include "kernel/types.h"
#include "user/user.h"

struct shared {
  int flag[2];
  int turn;
  int shared_counter;
};

static void
critical_section(volatile struct shared *s, int id)
{
  int other = 1 - id;
  s->flag[id] = 1;
  s->turn = other;
  while (s->flag[other] && s->turn == other)
    ;

  s->shared_counter++;
  printf("Peterson: process %d in CS, counter = %d\n", id,
         s->shared_counter);

  s->flag[id] = 0;
}

int
main(void)
{
  volatile struct shared *s = (volatile struct shared *)shm_get();
  if (s == 0) {
    printf("Peterson: shm_get failed\n");
    exit(1);
  }
  s->flag[0] = 0;
  s->flag[1] = 0;
  s->turn = 0;
  s->shared_counter = 0;

  int pid = fork();
  if (pid < 0) {
    printf("Peterson: fork failed\n");
    exit(1);
  }
  int id = pid == 0 ? 1 : 0;
  for (int i = 0; i < 10; i++) {
    critical_section(s, id);
    pause(1);
  }

  if (pid > 0) {
    wait(0);
    printf("Peterson: final counter = %d (expected 20)\n",
           s->shared_counter);
    exit(s->shared_counter == 20 ? 0 : 1);
  }
  exit(0);
}
