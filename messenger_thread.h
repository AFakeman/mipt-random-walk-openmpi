#include <stddef.h>

#include "simulation.h"

struct MessengerThread;
struct Particle;

typedef struct MessengerThread MessengerThread;

MessengerThread* MessengerThreadCreate(InitialParams* params,
                                       size_t bound,
                                       size_t width);

void MessengerThreadDelete(MessengerThread* self);

void MessengerThreadShutdown(MessengerThread* self);

void MessengerThreadJoin(MessengerThread*);

// Returns how many particles finished on other machines and
// resets the counter.
size_t MessengerThreadGetFinishedCount(MessengerThread* self);

// Get a pending particle from the queue. Returns NULL if the
// queue is empty right now.
Particle* MessengerThreadParticlePop(MessengerThread* self);

void MessengerThreadSendParticle(MessengerThread* self,
                                 Particle* particle,
                                 int target);

void MessengerThreadSendStats(MessengerThread* self, size_t delta);

void MessengerThreadDumpField(MessengerThread* self,
                              size_t* counts,
                              size_t bound);