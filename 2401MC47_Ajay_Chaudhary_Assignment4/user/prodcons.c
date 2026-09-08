#include "kernel/types.h"
#include "user/user.h"

#define ITEMS 20
#define DEFAULT_CAPACITY 5
#define MAX_CAPACITY 32

struct shared {
  int buffer[MAX_CAPACITY];
  int in;
  int out;
  int consumed[ITEMS];
  int consumed_count;
};

int
main(int argc, char **argv)
{
  int capacity = DEFAULT_CAPACITY;
  if (argc > 1)
    capacity = atoi(argv[1]);
  if (capacity < 1 || capacity > MAX_CAPACITY) {
    printf("ProdCons: capacity must be 1..%d\n", MAX_CAPACITY);
    exit(1);
  }

  struct shared *s = (struct shared *)shm_get();
  int empty = sem_create(capacity);
  int full = sem_create(0);
  int mutex = sem_create(1);
  if (s == 0 || empty < 0 || full < 0 || mutex < 0) {
    printf("ProdCons: setup failed\n");
    exit(1);
  }
  s->in = 0;
  s->out = 0;
  s->consumed_count = 0;

  int pid = fork();
  if (pid < 0) {
    printf("ProdCons: fork failed\n");
    exit(1);
  }
  if (pid > 0) {
    for (int value = 1; value <= ITEMS; value++) {
      sem_wait(empty);
      sem_wait(mutex);
      s->buffer[s->in] = value;
      s->in = (s->in + 1) % capacity;
      printf("Producer: inserted %d\n", value);
      sem_post(mutex);
      sem_post(full);
      pause(1);
    }
    wait(0);
    int ok = s->consumed_count == ITEMS;
    for (int i = 0; i < ITEMS; i++)
      if (s->consumed[i] != i + 1)
        ok = 0;
    printf("ProdCons: consumed %d items, order check %s\n",
           s->consumed_count, ok ? "PASS" : "FAIL");
    sem_destroy(empty);
    sem_destroy(full);
    sem_destroy(mutex);
    exit(ok ? 0 : 1);
  }

  for (int i = 0; i < ITEMS; i++) {
    sem_wait(full);
    sem_wait(mutex);
    int value = s->buffer[s->out];
    s->out = (s->out + 1) % capacity;
    s->consumed[s->consumed_count++] = value;
    printf("Consumer: removed %d\n", value);
    sem_post(mutex);
    sem_post(empty);
    pause(1);
  }
  exit(0);
}
