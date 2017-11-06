#include "simulation.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "fixed_list.h"
#include "messenger_thread.h"

static const int kMaxGraceBound = 10;
static const int kGraceScaleFactor = 10;
static const size_t kIterationsPerUpdate = 100;

Particle* ParticleCreate(int min_x, int min_y, int bound, int rank) {
  Particle* new = malloc(sizeof(Particle));
  new->x = min_x + (rand() % bound);
  new->y = min_y + (rand() % bound);
  new->parent = rank;
  new->iterations = 0;
  return new;
}

void MoveParticle(Particle* particle,
                  double p_l,
                  double p_r,
                  double p_u,
                  double p_d) {
  ++particle->iterations;
  double rand_num = rand() / (double)RAND_MAX;
  p_r += p_l;
  p_u += p_r;
  p_d += p_u;
  if (rand_num <= p_l) {
    --particle->x;
  } else if (rand_num <= p_r) {
    ++particle->x;
  } else if (rand_num <= p_u) {
    --particle->y;
  } else {
    ++particle->y;
  }
}

MessengerThread* CreateMsgThreadAndFillParams(InitialParams* params,
                                              size_t bound,
                                              size_t width) {
  pthread_mutex_init(&params->mtx, NULL);
  pthread_cond_init(&params->cond, NULL);
  atomic_init(&params->done);
  atomic_store(&params->done, 0);
  pthread_mutex_lock(&params->mtx);
  MessengerThread* thread = MessengerThreadCreate(params, bound, width);
  while (!atomic_load(&params->done))
    pthread_cond_wait(&params->cond, &params->mtx);
  pthread_mutex_unlock(&params->mtx);
  return thread;
}

size_t* arr3dget(size_t* arr,
                 size_t idx1,
                 size_t idx2,
                 size_t idx3,
                 size_t size1,
                 size_t size2,
                 size_t size3) {
  size_t offset = idx1 * size2 * size3 + idx2 * size3 + idx3;
  if (idx1 >= size1) {
    printf("idx1: %lu, size1: %lu\n", idx1, size1);
    assert(idx1 < size1);
  }
  if (idx2 >= size2) {
    printf("idx2: %lu, size2: %lu\n", idx2, size2);
    assert(idx2 < size2);
  }
  if (idx3 >= size3) {
    printf("idx3: %lu, size3: %lu\n", idx3, size3);
    assert(idx3 < size3);
  }
  return arr + offset;
}

void SimulationRun(size_t bound,
                   size_t width,
                   size_t height,
                   size_t max_iterations,
                   size_t start_particles,
                   double p_l,
                   double p_r,
                   double p_u,
                   double p_d) {
  size_t* finished_by_rank =
      (size_t*)malloc(sizeof(size_t) * bound * bound * width * height);
  size_t finished_particles = 0;
  size_t delta = 0;
  size_t total_particles = width * height * start_particles;
  size_t iterations = 0;
  FixedList* list = FixedListCreate(total_particles);
  InitialParams mpi_params;
  MessengerThread* msg_thread =
      CreateMsgThreadAndFillParams(&mpi_params, bound, width);
  assert(msg_thread);
  int grace_bound;
  const int x_pos = mpi_params.rank % width;
  const int y_pos = mpi_params.rank / width;
  const int min_x = x_pos * bound;
  const int min_y = y_pos * bound;
  const int max_x = min_x + bound - 1;
  const int max_y = min_y + bound - 1;
  if (bound / kGraceScaleFactor < kMaxGraceBound) {
    grace_bound = bound / kGraceScaleFactor;
  } else {
    grace_bound = kMaxGraceBound;
  }
  for (int x = 0; x < bound; ++x) {
    for (int y = 0; y < bound; ++y) {
      for (size_t i = 0; i < width * height; ++i) {
        *arr3dget(finished_by_rank, x, y, i, bound, bound, width * height) = 0;
      }
    }
  }
  for (size_t i = 0; i < start_particles; ++i) {
    FixedListPushFront(list,
                       ParticleCreate(min_x, min_y, bound, mpi_params.rank));
  }
  while (finished_particles < total_particles) {
    FixedListNode* prev = NULL;
    FixedListNode* cursor = FixedListBegin(list);
    while (cursor) {
      Particle* particle = (Particle*)cursor->data;
      MoveParticle(particle, p_l, p_r, p_u, p_d);
      int target_rank = mpi_params.rank;
      int target_x_pos = x_pos;
      int target_y_pos = y_pos;

      if (particle->x > max_x + grace_bound || 
          particle->x < min_x - grace_bound ||
          particle->y > max_y + grace_bound ||
          particle->y < min_y - grace_bound ||
          particle->iterations == max_iterations) {
        if (particle->x < 0)
          particle->x += bound * width;
        else if (particle->x >= bound * width)
          particle->x -= bound * width;
        if (particle->y < 0)
          particle->y += bound * height;
        else if (particle->y >= bound * height)
          particle->y -= bound * height;
        target_x_pos = particle->x / bound;
        target_y_pos = particle->y / bound;
      }

      target_rank = width * target_y_pos + target_x_pos;

      if (target_rank != mpi_params.rank) {
        if (!(particle->x >= target_x_pos * bound)) {
          printf("%d: particle->x: %d, target_x_pos: %d, particle->iterations: %lu\n", mpi_params.rank, particle->x, target_x_pos, particle->iterations);
          assert(particle->x >= target_x_pos * bound);
        }
        if (!(particle->x <= (target_x_pos + 1) * bound)) {
          printf("%d: particle->x: %d, target_x_pos: %d, particle->iterations: %lu\n", mpi_params.rank, particle->x, target_x_pos, particle->iterations);
          assert(particle->x <= (target_x_pos + 1) * bound);
        }
        if (!(particle->y >= target_y_pos * bound)) {
          printf("%d: particle->y: %d, target_y_pos: %d, particle->iterations: %lu\n", mpi_params.rank, particle->y, target_y_pos, particle->iterations);
          assert(particle->y >= target_y_pos * bound);
        }
        if (!(particle->y <= (target_y_pos + 1) * bound)) {
          printf("%d: particle->y: %d, target_y_pos: %d, particle->iterations: %lu\n", mpi_params.rank, particle->y, target_y_pos, particle->iterations);
          assert(particle->y <= (target_y_pos + 1) * bound);
        }
        FixedListDeleteElement(list, prev);
        MessengerThreadSendParticle(msg_thread, particle, target_rank);
        if (prev) {
          cursor = prev->next;
        } else {
          cursor = FixedListBegin(list);
        }
      } else if (particle->iterations == max_iterations) {
        if (!(particle->y - min_y >= 0)) {
          printf("particle->y - min_y: %d\n", particle->y - min_y);
        }
        if (!(particle->x - min_x >= 0)) {
          printf("particle->x - min_x > 0: %d\n", particle->x - min_x);
        }
        if (!(particle->x - min_x < bound)) {
          printf("particle->x - min_x < bound: %d\n", particle->x - min_x);
        }
        if (!(particle->y - min_y < bound)) {
          printf("particle->y - min_y < bound: %d\n", particle->y - min_y);
        }
        assert(particle->y - min_y >= 0);
        assert(particle->x - min_x >= 0);
        assert(particle->x - min_x < bound);
        assert(particle->y - min_y < bound);
        //printf("A particle died here\n");
        *arr3dget(finished_by_rank, particle->x - min_x, particle->y - min_y,
                  particle->parent, bound, bound, width * height) += 1;
        free(particle);
        cursor->data = NULL;
        FixedListDeleteElement(list, prev);
        if (prev) {
          cursor = prev->next;
        } else {
          cursor = FixedListBegin(list);
        }
        ++delta;
      } else {
        prev = cursor;
        cursor = prev->next;
      }
    }
    if (FixedListSize(list) == 0) {
      finished_particles += MessengerThreadGetFinishedCount(msg_thread);
      if (delta) {
        MessengerThreadSendStats(msg_thread, delta);
        delta = 0;
      }
    }
    ++iterations;
    if (iterations == kIterationsPerUpdate) {
      Particle* particle;
      iterations = 0;
      while ((particle = MessengerThreadParticlePop(msg_thread))) {
        if (particle->iterations == max_iterations) {
          //printf("Received a dead particle\n");
          if (!(particle->y - min_y >= 0)) {
            printf("%d: particle->x: %d, particle->y: %d, min_y: %d\n", mpi_params.rank, particle->x, particle->y, min_y);
            assert(particle->y - min_y >= 0);
          }
          if (!(particle->x - min_x >= 0)) {
            printf("%d: particle->x: %d, particle->y: %d, min_x: %d\n", mpi_params.rank, particle->x, particle->y, min_x);
            assert(particle->x - min_x >= 0);
          }
          if (!(particle->x - min_x < bound)) {
            printf("%d: particle->x: %d, particle->y: %d, min_x: %d\n", mpi_params.rank, particle->x, particle->y, min_x);
            assert(particle->x - min_x < bound);
          }
          if (!(particle->y - min_y < bound)) {
            printf("%d: particle->x: %d, particle->y: %d, min_y: %d\n", mpi_params.rank, particle->x, particle->y, min_y);
            assert(particle->y - min_y < bound);
          }
          *arr3dget(finished_by_rank, particle->x - min_x, particle->y - min_y,
                    particle->parent, bound, bound, width * height) += 1;
          ++delta;
          free(particle);
        } else {
          FixedListPushFront(list, particle);
        }
      }
    }
  }
  pthread_cond_destroy(&mpi_params.cond);
  pthread_mutex_destroy(&mpi_params.mtx);
  atomic_destroy(&mpi_params.done);
  MessengerThreadDumpField(msg_thread, finished_by_rank,
                           bound * bound * width * height);
  MessengerThreadShutdown(msg_thread);
  MessengerThreadJoin(msg_thread);
  MessengerThreadDelete(msg_thread);
  free(finished_by_rank);
  FixedListDelete(list);
}