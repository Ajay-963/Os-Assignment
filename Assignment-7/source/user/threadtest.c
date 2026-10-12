#include "kernel/types.h"
#include "user/user.h"

#define THREADS 3
#define INCREMENTS 100000

volatile int counter;

static void
increment(void *arg)
{
  (void)arg;
  for (int i = 0; i < INCREMENTS; i++)
    counter++;
}

int
main(void)
{
  void *stacks[THREADS];

  for (int i = 0; i < THREADS; i++) {
    stacks[i] = malloc(4096);
    if (stacks[i] == 0) {
      printf("Q1 FAIL: stack allocation\n");
      exit(1);
    }
    if (clone(increment, 0, (char *)stacks[i] + 4096) < 0) {
      printf("Q1 FAIL: clone\n");
      exit(1);
    }
  }
  for (int i = 0; i < THREADS; i++) {
    if (join() < 0) {
      printf("Q1 FAIL: join\n");
      exit(1);
    }
  }
  printf("Q1 unsynchronized counter=%d expected_without_race=%d\n", counter,
         THREADS * INCREMENTS);
  exit(0);
}
