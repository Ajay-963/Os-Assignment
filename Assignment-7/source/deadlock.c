#include "kernel/types.h"
#include "user/user.h"

#define PROCESSES 5
#define RESOURCES 3

static void
print_resources(const char *label, int resources[], int count)
{
  printf("%s", label);
  for (int r = 0; r < count; r++)
    printf(" %d", resources[r]);
  printf("\n");
}

static int
banker_safe(int allocation[PROCESSES][RESOURCES],
            int maximum[PROCESSES][RESOURCES], int available[RESOURCES],
            int sequence[PROCESSES], int process_count)
{
  int work[RESOURCES];
  int finished[PROCESSES] = {0};
  int count = 0;

  for (int r = 0; r < RESOURCES; r++)
    work[r] = available[r];
  print_resources("BANKER available:", work, RESOURCES);

  while (count < process_count) {
    int progressed = 0;
    for (int p = 0; p < process_count; p++) {
      int can_finish = !finished[p];
      for (int r = 0; r < RESOURCES && can_finish; r++)
        can_finish = maximum[p][r] - allocation[p][r] <= work[r];
      if (!can_finish)
        continue;
      for (int r = 0; r < RESOURCES; r++)
        work[r] += allocation[p][r];
      finished[p] = 1;
      sequence[count++] = p;
      printf("BANKER after P%d finishes, work:", p);
      for (int r = 0; r < RESOURCES; r++)
        printf(" %d", work[r]);
      printf("\n");
      progressed = 1;
    }
    if (!progressed) {
      print_resources("BANKER unsafe; remaining work:", work, RESOURCES);
      return 0;
    }
  }
  return 1;
}

static void
print_banker_state(const char *label,
                   int allocation[PROCESSES][RESOURCES],
                   int maximum[PROCESSES][RESOURCES],
                   int available[RESOURCES], int process_count)
{
  printf("%s\n", label);
  print_resources("  Available:", available, RESOURCES);
  for (int p = 0; p < process_count; p++) {
    printf("  P%d Allocation:", p);
    print_resources("", allocation[p], RESOURCES);
    printf("  P%d Need:", p);
    for (int r = 0; r < RESOURCES; r++)
      printf(" %d", maximum[p][r] - allocation[p][r]);
    printf("\n");
  }
}

static int
banker_request(int allocation[PROCESSES][RESOURCES],
               int maximum[PROCESSES][RESOURCES],
               int available[RESOURCES], int process, int request[RESOURCES],
               int sequence[PROCESSES], int process_count)
{
  for (int r = 0; r < RESOURCES; r++) {
    int need = maximum[process][r] - allocation[process][r];
    if (request[r] < 0 || request[r] > need || request[r] > available[r]) {
      printf("BANKER REQUEST rejected: exceeds Need or Available\n");
      return 0;
    }
  }

  for (int r = 0; r < RESOURCES; r++) {
    available[r] -= request[r];
    allocation[process][r] += request[r];
  }
  print_banker_state("State after hypothetical grant", allocation, maximum,
                     available, process_count);
  if (banker_safe(allocation, maximum, available, sequence, process_count)) {
    printf("BANKER REQUEST for P%d is safe to grant\n", process);
    return 1;
  }
  printf("BANKER REQUEST for P%d is unsafe; do not grant\n", process);
  return -1;
}

static int
detect_deadlock(int allocation[2][2], int request[2][2], int available[2],
                int deadlocked[2])
{
  int work[2] = {available[0], available[1]};
  int finished[2];
  int count = 0;

  print_resources("DETECT available:", work, 2);
  for (int p = 0; p < 2; p++)
    finished[p] = allocation[p][0] == 0 && allocation[p][1] == 0;
  for (;;) {
    int progressed = 0;
    for (int p = 0; p < 2; p++) {
      if (finished[p] || request[p][0] > work[0] ||
          request[p][1] > work[1])
        continue;
      work[0] += allocation[p][0];
      work[1] += allocation[p][1];
      finished[p] = 1;
      printf("DETECT after P%d finishes, work: %d %d\n", p, work[0],
             work[1]);
      progressed = 1;
    }
    if (!progressed)
      break;
  }
  printf("DETECT unfinished:");
  for (int p = 0; p < 2; p++) {
    if (!finished[p]) {
      printf(" P%d", p);
      deadlocked[count++] = p;
    }
  }
  if (count == 0)
    printf(" none");
  printf("\n");
  print_resources("DETECT remaining work:", work, 2);
  return count;
}

static void
wait_for_resource(int resource_fd, int requested_fd, int ready_read_fd,
                  int ready_write_fd, int gate_read_fd, int gate_write_fd)
{
  char token;

  close(ready_read_fd);
  close(gate_write_fd);
  if (read(resource_fd, &token, 1) != 1)
    exit(1);
  if (write(ready_write_fd, "r", 1) != 1)
    exit(1);
  if (read(gate_read_fd, &token, 1) != 1)
    exit(1);
  read(requested_fd, &token, 1);
  exit(0);
}

static int
actual_process_deadlock(void)
{
  int resource_a[2], resource_b[2], ready[2], gate[2];
  char token = 'x';
  char ready_byte;
  int pids[2];

  if (pipe(resource_a) < 0 || pipe(resource_b) < 0 || pipe(ready) < 0 ||
      pipe(gate) < 0)
    return -1;
  if (write(resource_a[1], &token, 1) != 1 ||
      write(resource_b[1], &token, 1) != 1)
    return -1;

  pids[0] = fork();
  if (pids[0] == 0)
    wait_for_resource(resource_a[0], resource_b[0], ready[0], ready[1],
                      gate[0], gate[1]);
  if (pids[0] < 0)
    return -1;

  pids[1] = fork();
  if (pids[1] == 0)
    wait_for_resource(resource_b[0], resource_a[0], ready[0], ready[1],
                      gate[0], gate[1]);
  if (pids[1] < 0) {
    kill(pids[0]);
    wait(0);
    return -1;
  }

  close(ready[1]);
  close(gate[0]);
  if (read(ready[0], &ready_byte, 1) != 1 ||
      read(ready[0], &ready_byte, 1) != 1) {
    kill(pids[0]);
    kill(pids[1]);
    wait(0);
    wait(0);
    return -1;
  }

  if (write(gate[1], "ab", 2) != 2) {
    kill(pids[0]);
    kill(pids[1]);
    wait(0);
    wait(0);
    return -1;
  }
  pause(3);
  printf("ACTUAL-PROCESS CASE: P%d holds A/waits for B; P%d holds B/waits for A\n",
         pids[0], pids[1]);
  kill(pids[0]);
  kill(pids[1]);
  wait(0);
  wait(0);

  close(resource_a[0]);
  close(resource_a[1]);
  close(resource_b[0]);
  close(resource_b[1]);
  close(ready[0]);
  close(gate[1]);
  return 0;
}

int
main(void)
{
  int allocation[PROCESSES][RESOURCES] = {
    {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}
  };
  int maximum[PROCESSES][RESOURCES] = {
    {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}
  };
  int safe_available[RESOURCES] = {3, 3, 2};
  int unsafe_available[RESOURCES] = {0, 0, 0};
  int sequence[PROCESSES];

  if (!banker_safe(allocation, maximum, safe_available, sequence, PROCESSES)) {
    printf("CASE 1 FAIL: expected Banker's safe state\n");
    exit(1);
  }
  printf("CASE 1 PASS: Banker's safe sequence:");
  for (int i = 0; i < PROCESSES; i++)
    printf(" P%d", sequence[i]);
  printf("\n");

  if (banker_safe(allocation, maximum, unsafe_available, sequence,
                  PROCESSES)) {
    printf("CASE 2 FAIL: expected Banker's unsafe state\n");
    exit(1);
  }
  printf("CASE 2 PASS: Banker's unsafe state has no safe sequence\n");

  int request_allocation[PROCESSES][RESOURCES] = {
    {1, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
  };
  int request_maximum[PROCESSES][RESOURCES] = {
    {2, 0, 0}, {3, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
  };
  int request_available[RESOURCES] = {1, 0, 0};
  int request[RESOURCES] = {0, 0, 0};
  print_banker_state("CASE 3 initial state", request_allocation,
                     request_maximum, request_available, 2);
  if (!banker_safe(request_allocation, request_maximum, request_available,
                   sequence, 2)) {
    printf("CASE 3 FAIL: initial request state is not safe\n");
    exit(1);
  }
  printf("CASE 3 initial state PASS: safe\n");

  request[0] = 1;
  printf("CASE 3 hypothetical request P1:");
  print_resources("", request, RESOURCES);
  int granted_allocation[PROCESSES][RESOURCES];
  int granted_maximum[PROCESSES][RESOURCES];
  int granted_available[RESOURCES];
  for (int p = 0; p < PROCESSES; p++)
    for (int r = 0; r < RESOURCES; r++) {
      granted_allocation[p][r] = request_allocation[p][r];
      granted_maximum[p][r] = request_maximum[p][r];
    }
  for (int r = 0; r < RESOURCES; r++)
    granted_available[r] = request_available[r];
  if (request[0] > request_maximum[1][0] - request_allocation[1][0] ||
      request[0] > request_available[0]) {
    printf("CASE 3 FAIL: hypothetical request is invalid\n");
    exit(1);
  }
  printf("CASE 3 request validity: 1 <= Need[1]=2 and Available=1\n");
  int request_result =
      banker_request(granted_allocation, granted_maximum, granted_available, 1,
                     request, sequence, 2);
  if (request_result != -1) {
    printf("CASE 3 FAIL: expected valid request to make state unsafe\n");
    exit(1);
  }
  printf("CASE 3 PASS: valid hypothetical request makes safe state unsafe\n");

  int allocation_two[2][2] = {{1, 0}, {0, 1}};
  int no_requests[2][2] = {{0, 0}, {0, 0}};
  int available[2] = {0, 0};
  int deadlocked[2];
  if (detect_deadlock(allocation_two, no_requests, available, deadlocked) !=
      0) {
    printf("CASE 4 FAIL: detector reported a deadlock\n");
    exit(1);
  }
  printf("CASE 4 PASS: deadlock detector found no deadlock\n");

  int requests[2][2] = {{0, 1}, {1, 0}};
  int count =
      detect_deadlock(allocation_two, requests, available, deadlocked);
  if (count != 2 || deadlocked[0] != 0 || deadlocked[1] != 1) {
    printf("CASE 5 FAIL: detector did not identify P0 and P1\n");
    exit(1);
  }
  printf("CASE 5 PASS: deadlock detector identified P0 and P1\n");
  if (actual_process_deadlock() < 0) {
    printf("ACTUAL-PROCESS CASE FAIL: pipe/fork/cleanup error\n");
    exit(1);
  }
  printf("ACTUAL-PROCESS CASE PASS: circular wait was bounded and terminated\n");
  exit(0);
}
