#include "kernel/types.h"
#include "user/user.h"

#define THREADS 3
#define INCREMENTS 100000
#define BUFFER_SIZE 5
#define ITEMS 50
#define SEM_COUNTER 0
#define SEM_EMPTY 1
#define SEM_FULL 2
#define SEM_BUFFER 3
#define SEM_LOG 4

volatile int counter;
int buffer[BUFFER_SIZE];
int put_index;
int get_index;
int occupied;
int overflow;
int underflow;

static void
increment(void *arg)
{
  (void)arg;
  for (int i = 0; i < INCREMENTS; i++) {
    sem_wait(SEM_COUNTER);
    counter++;
    sem_signal(SEM_COUNTER);
  }
}

static void
produce(void *arg)
{
  (void)arg;
  for (int i = 0; i < ITEMS; i++) {
    int slot;
    sem_wait(SEM_EMPTY);
    sem_wait(SEM_BUFFER);
    if (occupied >= BUFFER_SIZE)
      overflow++;
    slot = put_index;
    buffer[slot] = i;
    put_index = (put_index + 1) % BUFFER_SIZE;
    occupied++;
    sem_signal(SEM_BUFFER);
    sem_wait(SEM_LOG);
    if (i < 5 || i == ITEMS - 1)
    printf("produced item=%d slot=%d\n", i, slot);
    sem_signal(SEM_LOG);
    sem_signal(SEM_FULL);
  }
}

static void
consume(void *arg)
{
  (void)arg;
  for (int i = 0; i < ITEMS; i++) {
    int item;
    int slot;
    sem_wait(SEM_FULL);
    sem_wait(SEM_BUFFER);
    if (occupied <= 0)
      underflow++;
    slot = get_index;
    item = buffer[slot];
    get_index = (get_index + 1) % BUFFER_SIZE;
    occupied--;
    sem_signal(SEM_BUFFER);
    sem_wait(SEM_LOG);
    if (i < 5 || i == ITEMS - 1)
    printf("consumed item=%d slot=%d\n", item, slot);
    sem_signal(SEM_LOG);
    sem_signal(SEM_EMPTY);
  }
}

int
main(void)
{
  void *stacks[THREADS + 2];
  for (int i = 0; i < THREADS + 2; i++) {
    stacks[i] = malloc(4096);
    if (stacks[i] == 0) {
      printf("Q2 FAIL: stack allocation\n");
      exit(1);
    }
  }
  if (sem_init(SEM_COUNTER, 1) < 0 ||
      sem_init(SEM_EMPTY, BUFFER_SIZE) < 0 ||
      sem_init(SEM_FULL, 0) < 0 ||
      sem_init(SEM_BUFFER, 1) < 0 ||
      sem_init(SEM_LOG, 1) < 0) {
    printf("Q2 FAIL: semaphore initialization\n");
    exit(1);
  }
  for (int i = 0; i < THREADS; i++) {
    if (clone(increment, 0, (char *)stacks[i] + 4096) < 0) {
      printf("Q2 FAIL: increment clone\n");
      exit(1);
    }
  }
  if (clone(produce, 0, (char *)stacks[THREADS] + 4096) < 0 ||
      clone(consume, 0, (char *)stacks[THREADS + 1] + 4096) < 0) {
    printf("Q2 FAIL: producer/consumer clone\n");
    exit(1);
  }
  for (int i = 0; i < THREADS + 2; i++) {
    if (join() < 0) {
      printf("Q2 FAIL: join\n");
      exit(1);
    }
  }

  if (counter != THREADS * INCREMENTS || occupied != 0 || overflow != 0 ||
      underflow != 0) {
    printf("Q2 FAIL counter=%d occupied=%d overflow=%d underflow=%d\n",
           counter, occupied, overflow, underflow);
    exit(1);
  }
  printf("Q2 PASS counter=%d buffer=%d overflow=%d underflow=%d\n",
         counter, occupied, overflow, underflow);
  exit(0);
}
