#include <stddef.h>

#include "atomic.h"

struct QueueNode;

typedef struct Queue {
  struct QueueNode* top_;
  struct QueueNode* back_;
  atomic_size_t size_;
} Queue;

// Initialize an empty queue.
void QueueInit(Queue* self);

// Destroy a queue.
void QueueDestroy(Queue* self);

// Return a topmost queue element without removing it
// from the queue. Empty queue returns NULL.
void* QueueTop(Queue* self);

size_t QueueSize(Queue* self);

// Return a topmost queue element, removing it from the queue.
// Empty queue returns NULL.
void* QueuePop(Queue* self);

// Add an element to the top of the queue.
void QueuePush(Queue* self, void* value);

// Returns 1 if the queue is empty, 0 otherwise.
int QueueEmpty(Queue* self);
