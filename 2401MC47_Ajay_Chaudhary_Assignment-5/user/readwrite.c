#include "kernel/types.h"
#include "user/user.h"

struct shared {
  int shared_data;
  int read_count;
  int readers_inside;
  int writer_inside;
};

static void
reader(struct shared *s, int rmutex, int resource, int turnstile, int id)
{
  for (int i = 0; i < 3; i++) {
    sem_wait(turnstile);
    sem_post(turnstile);
    sem_wait(rmutex);
    s->read_count++;
    if (s->read_count == 1)
      sem_wait(resource);
    s->readers_inside++;
    sem_post(rmutex);

    printf("Reader %d (pid %d): ENTER data=%d readers=%d\n", id, getpid(),
           s->shared_data, s->readers_inside);
    pause(2);
    printf("Reader %d (pid %d): EXIT\n", id, getpid());

    sem_wait(rmutex);
    s->readers_inside--;
    s->read_count--;
    if (s->read_count == 0)
      sem_post(resource);
    sem_post(rmutex);
    pause(1);
  }
}

static void
writer(struct shared *s, int resource, int turnstile, int id)
{
  for (int i = 0; i < 3; i++) {
    sem_wait(turnstile);
    sem_wait(resource);
    sem_post(turnstile);

    s->writer_inside++;
    printf("Writer %d (pid %d): ENTER data=%d\n", id, getpid(),
           s->shared_data);
    s->shared_data++;
    printf("Writer %d (pid %d): UPDATED data=%d\n", id, getpid(),
           s->shared_data);
    s->writer_inside--;
    printf("Writer %d (pid %d): EXIT\n", id, getpid());
    sem_post(resource);
    pause(1);
  }
}

int
main(void)
{
  struct shared *s = (struct shared *)shm_get();
  int rmutex = sem_create(1);
  int resource = sem_create(1);
  int turnstile = sem_create(1);
  if (s == 0 || rmutex < 0 || resource < 0 || turnstile < 0) {
    printf("ReadWrite: setup failed\n");
    exit(1);
  }
  s->shared_data = 0;
  s->read_count = 0;
  s->readers_inside = 0;
  s->writer_inside = 0;

  for (int i = 0; i < 5; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("ReadWrite: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      if (i < 3)
        reader(s, rmutex, resource, turnstile, i);
      else
        writer(s, resource, turnstile, i - 3);
      exit(0);
    }
  }
  for (int i = 0; i < 5; i++)
    wait(0);
  printf("ReadWrite: final shared_data=%d, readers=%d, writers=%d\n",
         s->shared_data, s->readers_inside, s->writer_inside);
  sem_destroy(rmutex);
  sem_destroy(resource);
  sem_destroy(turnstile);
  exit(0);
}
