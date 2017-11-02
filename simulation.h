#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>

#pragma once

typedef struct Particle {
  int x;
  int y;
  int parent;
  size_t iterations;
} Particle;

typedef struct InitialParams {
  atomic_int done;
  pthread_mutex_t mtx;
  pthread_cond_t cond;
  int rank;
} InitialParams;

void SimulationRun(size_t l,
                   size_t a,
                   size_t b,
                   size_t n,
                   size_t N,
                   double p_l,
                   double p_r,
                   double p_u,
                   double p_d);