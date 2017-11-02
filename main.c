#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

#include "simulation.h"

int main(int argc, char* argv[]) {
  int support;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &support);
  size_t l;
  size_t a;
  size_t b;
  size_t n;
  size_t N;
  double p_l;
  double p_r;
  double p_u;
  double p_d;
  assert(argc == 10);
  assert(sscanf(argv[1], "%lu", &l));
  assert(sscanf(argv[2], "%lu", &a));
  assert(sscanf(argv[3], "%lu", &b));
  assert(sscanf(argv[4], "%lu", &n));
  assert(sscanf(argv[5], "%lu", &N));
  assert(sscanf(argv[6], "%lf", &p_l));
  assert(sscanf(argv[7], "%lf", &p_r));
  assert(sscanf(argv[8], "%lf", &p_u));
  assert(sscanf(argv[9], "%lf", &p_d));
  SimulationRun(l, a, b, n, N, p_l, p_r, p_u, p_d);
  MPI_Finalize();
}
