#include "messenger_thread.h"

#include <mpi.h>
#include <pthread.h>
#include <stdlib.h>

#include "atomic.h"
#include "queue.h"

static MPI_Datatype MPI_Particle;
static MPI_Datatype MPI_Particle_Vector;

static const char* kDumpFilename = "data.bin";

struct MessengerThread {
  pthread_t thread_;
  Queue send_queue_;
  pthread_mutex_t send_queue_mtx_;
  Queue receive_queue_;
  pthread_mutex_t receive_queue_mtx_;
  atomic_size_t finished_count_;
  atomic_size_t shutdown_;
  size_t bound;
  size_t width;
  int rank;
  int size;
};

typedef struct MessengerThreadParams {
  InitialParams* master_params;
  MessengerThread* self;
} MessengerThreadParams;

typedef enum { PARTICLE, COUNT, DUMP } MessengerThreadMessageId;

typedef struct OutgoingMessage {
  MessengerThreadMessageId type;
  union {
    size_t count;
    struct {
      Particle* particle;
      int destination;
    } particle;
    struct {
      size_t* counts;
      size_t length;
    } dump;
  } value;
} OutgoingMessage;

void InitializeStructure(InitialParams* params) {
  pthread_mutex_lock(&params->mtx);
  MPI_Comm_rank(MPI_COMM_WORLD, &params->rank);
  pthread_mutex_unlock(&params->mtx);
  atomic_store(&params->done, 1);
  pthread_cond_signal(&params->cond);
}

OutgoingMessage* PopMessage(MessengerThread* self) {
  if (atomic_load(&self->send_queue_.size_) == 0) {
    return NULL;
  }
  pthread_mutex_lock(&self->send_queue_mtx_);
  OutgoingMessage* to_return = (OutgoingMessage*)QueuePop(&self->send_queue_);
  pthread_mutex_unlock(&self->send_queue_mtx_);
  return to_return;
}

void DumpData(MessengerThread* self, OutgoingMessage* msg) {
  MPI_File file;
  MPI_Aint inset;
  MPI_Aint unused;
  MPI_Type_get_extent(MPI_UNSIGNED_LONG_LONG, &unused, &inset);
  MPI_File_open(MPI_COMM_WORLD, kDumpFilename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                MPI_INFO_NULL, &file);
  size_t x_pos = self->rank % self->width;
  size_t y_pos = self->rank / self->width;

  MPI_Offset offset = y_pos * self->width * msg->value.dump.length +
                      x_pos * self->bound * self->size;
  offset *= inset;
  MPI_File_set_view(file, offset, MPI_UNSIGNED_LONG_LONG, MPI_Particle_Vector,
                    "native", MPI_INFO_NULL);
  MPI_File_write_all(file, msg->value.dump.counts, msg->value.dump.length,
                     MPI_UNSIGNED_LONG_LONG, MPI_STATUS_IGNORE);
  MPI_File_close(&file);
}

void SendMessage(MessengerThread* self, OutgoingMessage* msg) {
  MPI_Request req;
  switch (msg->type) {
    case PARTICLE: {
      MPI_Isend(msg->value.particle.particle, 1, MPI_Particle,
                msg->value.particle.destination, PARTICLE, MPI_COMM_WORLD,
                &req);
      free(msg->value.particle.particle);
      MPI_Request_free(&req);
      break;
    }
    case COUNT: {
      for (int i = 0; i < self->size; ++i) {
        MPI_Isend(&msg->value, 1, MPI_UNSIGNED_LONG_LONG, i, COUNT,
                  MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);
      }
      break;
    }
    case DUMP: {
      DumpData(self, msg);
    }
  }

  free(msg);
}

void InitMPIStruct(size_t size, size_t bound, size_t width) {
  {
    int block_lengths[4] = {1, 1, 1, 1};
    MPI_Aint offsets[4] = {offsetof(Particle, x), offsetof(Particle, y),
                           offsetof(Particle, parent),
                           offsetof(Particle, iterations)};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_UNSIGNED_LONG_LONG};
    MPI_Type_create_struct(4, block_lengths, offsets, types, &MPI_Particle);
    MPI_Type_commit(&MPI_Particle);
  }
  {
    MPI_Type_vector(bound, bound * size, (width - 1) * bound * size * 2,
                    MPI_UNSIGNED_LONG_LONG, &MPI_Particle_Vector);
    MPI_Type_commit(&MPI_Particle_Vector);
  }
}

void* MessengerThreadJob(void* in) {
  MessengerThreadParams* params = (MessengerThreadParams*)in;
  MessengerThread* self = params->self;
  MPI_Comm_rank(MPI_COMM_WORLD, &self->rank);
  MPI_Comm_size(MPI_COMM_WORLD, &self->size);
  InitMPIStruct(self->size, self->bound, self->width);
  InitializeStructure(params->master_params);
  while (!(atomic_load(&self->shutdown_) && QueueEmpty(&self->receive_queue_) &&
           QueueEmpty(&self->send_queue_))) {
    OutgoingMessage* out_msg;
    while ((out_msg = PopMessage(self))) {
      SendMessage(self, out_msg);
    }
    int flag = 0;
    for (int i = 0; i < self->size; ++i) {
      MPI_Iprobe(i, COUNT, MPI_COMM_WORLD, &flag, NULL);
      if (flag) {
        size_t count;
        MPI_Recv(&count, 1, MPI_UNSIGNED_LONG_LONG, i, COUNT, MPI_COMM_WORLD,
                 NULL);
        atomic_fetch_add(&self->finished_count_, count);
        flag = 0;
      }
    }
    for (int i = 0; i < self->size; ++i) {
      MPI_Iprobe(i, PARTICLE, MPI_COMM_WORLD, &flag, NULL);
      if (flag) {
        Particle* particle = (Particle*)malloc(sizeof(Particle));
        MPI_Recv(particle, 1, MPI_Particle, i, PARTICLE, MPI_COMM_WORLD, NULL);
        pthread_mutex_lock(&self->receive_queue_mtx_);
        QueuePush(&self->receive_queue_, particle);
        pthread_mutex_unlock(&self->receive_queue_mtx_);
        flag = 0;
      }
    }
  }
  return NULL;
}

MessengerThread* MessengerThreadCreate(InitialParams* params,
                                       size_t bound,
                                       size_t width) {
  MessengerThread* self = (MessengerThread*)malloc(sizeof(MessengerThread));
  MessengerThreadParams* job_params =
      (MessengerThreadParams*)malloc(sizeof(MessengerThreadParams));
  job_params->self = self;
  job_params->master_params = params;
  pthread_mutex_init(&self->send_queue_mtx_, NULL);
  pthread_mutex_init(&self->receive_queue_mtx_, NULL);
  QueueInit(&self->send_queue_);
  QueueInit(&self->receive_queue_);
  atomic_init(&self->finished_count_);
  atomic_init(&self->shutdown_);
  atomic_store(&self->finished_count_, 0);
  atomic_store(&self->shutdown_, 0);
  self->bound = bound;
  self->width = width;
  pthread_create(&self->thread_, NULL, MessengerThreadJob, job_params);
  return self;
};

void MessengerThreadDelete(MessengerThread* self) {
  pthread_mutex_destroy(&self->send_queue_mtx_);
  pthread_mutex_destroy(&self->receive_queue_mtx_);
  atomic_destroy(&self->finished_count_);
  atomic_destroy(&self->shutdown_);
  QueueDestroy(&self->receive_queue_);
  QueueDestroy(&self->send_queue_);
  free(self);
}

void MessengerThreadShutdown(MessengerThread* self) {
  atomic_store(&self->shutdown_, 1);
}

void MessengerThreadJoin(MessengerThread* self) {
  pthread_join(self->thread_, NULL);
}

size_t MessengerThreadGetFinishedCount(MessengerThread* self) {
  return atomic_exchange(&self->finished_count_, 0);
}

Particle* MessengerThreadParticlePop(MessengerThread* self) {
  pthread_mutex_lock(&self->receive_queue_mtx_);
  Particle* to_return = (Particle*)QueuePop(&self->receive_queue_);
  pthread_mutex_unlock(&self->receive_queue_mtx_);
  return to_return;
}

void MessengerThreadSendParticle(MessengerThread* self,
                                 Particle* particle,
                                 int target) {
  OutgoingMessage* msg = (OutgoingMessage*)malloc(sizeof(OutgoingMessage));
  msg->type = PARTICLE;
  msg->value.particle.particle = particle;
  msg->value.particle.destination = target;
  pthread_mutex_lock(&self->send_queue_mtx_);
  QueuePush(&self->send_queue_, msg);
  pthread_mutex_unlock(&self->send_queue_mtx_);
}

void MessengerThreadSendStats(MessengerThread* self, size_t delta) {
  OutgoingMessage* msg = (OutgoingMessage*)malloc(sizeof(OutgoingMessage));
  msg->type = COUNT;
  msg->value.count = delta;
  pthread_mutex_lock(&self->send_queue_mtx_);
  QueuePush(&self->send_queue_, msg);
  pthread_mutex_unlock(&self->send_queue_mtx_);
}

void MessengerThreadDumpField(MessengerThread* self,
                              size_t* counts,
                              size_t length) {
  OutgoingMessage* msg = (OutgoingMessage*)malloc(sizeof(OutgoingMessage));
  msg->type = DUMP;
  msg->value.dump.length = length;
  msg->value.dump.counts = counts;
  pthread_mutex_lock(&self->send_queue_mtx_);
  QueuePush(&self->send_queue_, msg);
  pthread_mutex_unlock(&self->send_queue_mtx_);
}