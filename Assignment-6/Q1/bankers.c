#include "kernel/types.h"
#include "user/user.h"

#define PROCESSES 5
#define RESOURCES 3

static int allocation[PROCESSES][RESOURCES] = {
  {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
static int maximum[PROCESSES][RESOURCES] = {
  {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
static int available[RESOURCES] = {3, 3, 2};

static void
print_matrix(char *name, int matrix[PROCESSES][RESOURCES])
{
  printf("%s:\n", name);
  for (int i = 0; i < PROCESSES; i++)
    printf("  P%d: %d %d %d\n", i, matrix[i][0], matrix[i][1], matrix[i][2]);
}

static void
compute_need(int need[PROCESSES][RESOURCES])
{
  for (int i = 0; i < PROCESSES; i++)
    for (int j = 0; j < RESOURCES; j++)
      need[i][j] = maximum[i][j] - allocation[i][j];
}

static int
can_finish(int process, int work[RESOURCES], int need[PROCESSES][RESOURCES])
{
  for (int j = 0; j < RESOURCES; j++)
    if (need[process][j] > work[j])
      return 0;
  return 1;
}

static int
safety(int sequence[PROCESSES])
{
  int need[PROCESSES][RESOURCES];
  int work[RESOURCES];
  int finished[PROCESSES];
  int count = 0;

  compute_need(need);
  for (int j = 0; j < RESOURCES; j++)
    work[j] = available[j];
  for (int i = 0; i < PROCESSES; i++)
    finished[i] = 0;

  while (count < PROCESSES) {
    int progress = 0;
    for (int i = 0; i < PROCESSES; i++) {
      if (!finished[i] && can_finish(i, work, need)) {
        for (int j = 0; j < RESOURCES; j++)
          work[j] += allocation[i][j];
        finished[i] = 1;
        sequence[count++] = i;
        progress = 1;
      }
    }
    if (!progress)
      return 0;
  }
  return 1;
}

static int
request_resources(int process, int request[RESOURCES])
{
  int need[PROCESSES][RESOURCES];
  int sequence[PROCESSES];

  compute_need(need);
  printf("Request from P%d: %d %d %d\n", process, request[0], request[1],
         request[2]);
  for (int j = 0; j < RESOURCES; j++) {
    if (request[j] > need[process][j]) {
      printf("Request denied - exceeds Need[P%d]\n", process);
      return 0;
    }
    if (request[j] > available[j]) {
      printf("Request denied - exceeds Available\n");
      return 0;
    }
  }

  for (int j = 0; j < RESOURCES; j++) {
    available[j] -= request[j];
    allocation[process][j] += request[j];
  }
  if (safety(sequence)) {
    printf("Request granted; resulting state is safe\n");
    printf("Safe sequence after grant: ");
    for (int i = 0; i < PROCESSES; i++)
      printf("P%d%s", sequence[i], i + 1 == PROCESSES ? "\n" : " -> ");
    return 1;
  }

  for (int j = 0; j < RESOURCES; j++) {
    available[j] += request[j];
    allocation[process][j] -= request[j];
  }
  printf("Request denied - would lead to unsafe state\n");
  printf("Temporary allocation rolled back\n");
  return 0;
}

int
main(void)
{
  int need[PROCESSES][RESOURCES];
  int sequence[PROCESSES];
  int granted_request[RESOURCES] = {1, 0, 2};
  int denied_request[RESOURCES] = {2, 3, 0};

  printf("Banker's Algorithm Simulation\n");
  print_matrix("Allocation", allocation);
  print_matrix("Max", maximum);
  printf("Available: %d %d %d\n", available[0], available[1], available[2]);
  compute_need(need);
  print_matrix("Need (Max - Allocation)", need);

  if (safety(sequence)) {
    printf("Initial state: SAFE\nSafe sequence: ");
    for (int i = 0; i < PROCESSES; i++)
      printf("P%d%s", sequence[i], i + 1 == PROCESSES ? "\n" : " -> ");
  } else {
    printf("Initial state: UNSAFE\n");
  }

  printf("\nScenario 1 (safe request)\n");
  request_resources(1, granted_request);
  printf("Available after scenario 1: %d %d %d\n", available[0], available[1],
         available[2]);

  printf("\nScenario 2 (unsafe request)\n");
  request_resources(4, denied_request);
  printf("Available after scenario 2: %d %d %d\n", available[0], available[1],
         available[2]);
  exit(0);
}
