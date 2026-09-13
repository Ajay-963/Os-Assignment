#include "kernel/types.h"
#include "user/user.h"

#define PROCESSES 4
#define RESOURCES 3

static void
print_matrix(char *name, int matrix[PROCESSES][RESOURCES])
{
  printf("%s:\n", name);
  for (int i = 0; i < PROCESSES; i++)
    printf("  P%d: %d %d %d\n", i, matrix[i][0], matrix[i][1], matrix[i][2]);
}

static void
build_graph(int allocation[PROCESSES][RESOURCES],
            int request[PROCESSES][RESOURCES],
            int graph[PROCESSES][PROCESSES])
{
  for (int i = 0; i < PROCESSES; i++)
    for (int j = 0; j < PROCESSES; j++)
      graph[i][j] = 0;

  for (int waiter = 0; waiter < PROCESSES; waiter++)
    for (int resource = 0; resource < RESOURCES; resource++)
      if (request[waiter][resource])
        for (int holder = 0; holder < PROCESSES; holder++)
          if (allocation[holder][resource])
            graph[waiter][holder] = 1;
}

static void
print_graph(int graph[PROCESSES][PROCESSES])
{
  printf("Wait-for graph edges:\n");
  for (int i = 0; i < PROCESSES; i++)
    for (int j = 0; j < PROCESSES; j++)
      if (graph[i][j])
        printf("  P%d -> P%d\n", i, j);
}

static int
dfs(int vertex, int graph[PROCESSES][PROCESSES], int state[PROCESSES],
    int path[PROCESSES], int depth, int cycle[PROCESSES], int *cycle_length)
{
  state[vertex] = 1;
  path[depth++] = vertex;
  for (int next = 0; next < PROCESSES; next++) {
    if (!graph[vertex][next])
      continue;
    if (state[next] == 0 &&
        dfs(next, graph, state, path, depth, cycle, cycle_length))
      return 1;
    if (state[next] == 1) {
      int start = 0;
      while (path[start] != next)
        start++;
      *cycle_length = 0;
      for (int i = start; i < depth; i++)
        cycle[(*cycle_length)++] = path[i];
      cycle[(*cycle_length)++] = next;
      return 1;
    }
  }
  state[vertex] = 2;
  return 0;
}

static void
run_scenario(char *name, int allocation[PROCESSES][RESOURCES],
             int request[PROCESSES][RESOURCES])
{
  int graph[PROCESSES][PROCESSES];
  int state[PROCESSES] = {0};
  int path[PROCESSES];
  int cycle[PROCESSES + 1];
  int cycle_length = 0;
  int found = 0;

  printf("\n%s\n", name);
  print_matrix("Allocation", allocation);
  print_matrix("Request", request);
  build_graph(allocation, request, graph);
  print_graph(graph);
  for (int i = 0; i < PROCESSES && !found; i++)
    if (state[i] == 0)
      found = dfs(i, graph, state, path, 0, cycle, &cycle_length);
  if (!found) {
    printf("Result: NO DEADLOCK (graph is acyclic)\n");
    return;
  }
  printf("Result: DEADLOCK DETECTED\nCycle: ");
  for (int i = 0; i < cycle_length; i++)
    printf("P%d%s", cycle[i], i + 1 == cycle_length ? "\n" : " -> ");
}

int
main(void)
{
  int no_deadlock_allocation[PROCESSES][RESOURCES] = {
    {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
  int no_deadlock_request[PROCESSES][RESOURCES] = {
    {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {1, 0, 0}};
  int deadlock_allocation[PROCESSES][RESOURCES] = {
    {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
  int deadlock_request[PROCESSES][RESOURCES] = {
    {0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, 0}};

  printf("Resource Allocation Graph Deadlock Detection\n");
  run_scenario("Scenario A: acyclic wait-for graph", no_deadlock_allocation,
               no_deadlock_request);
  run_scenario("Scenario B: three-process circular wait", deadlock_allocation,
               deadlock_request);
  exit(0);
}
