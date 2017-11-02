#include "queue.h"

#include <assert.h>
#include <stdlib.h>

struct QueueNode {
  void* data;
  struct QueueNode* next;
};

typedef struct QueueNode QueueNode;

void QueueInit(Queue* self) {
  self->top_ = NULL;
  self->back_ = NULL;
  atomic_init(&self->size_);
  atomic_store(&self->size_, 0);
}

void QueueDestroy(Queue* self) {
  if (!self->top_)
    return;
  QueueNode* cursor = self->top_->next;
  QueueNode* to_delete = self->top_;
  while (cursor != NULL) {
    free(to_delete);
    to_delete = cursor;
    cursor = cursor->next;
  }
  atomic_destroy(&self->size_);
  free(to_delete);
}

void* QueueTop(Queue* self) {
  if (!self->top_)
    return NULL;
  else
    return self->top_->data;
}

size_t QueueSize(Queue* self) {
  return atomic_load(&self->size_);
}

void* QueuePop(Queue* self) {
  if (!self->top_) {
    return NULL;
  }
  void* data = self->top_->data;
  QueueNode* to_cleanup = self->top_;
  self->top_ = to_cleanup->next;
  if (!self->top_)
    self->back_ = NULL;
  free(to_cleanup);
  atomic_fetch_sub(&self->size_, 1);
  return data;
}

void QueuePush(Queue* self, void* data) {
  QueueNode* new_back = malloc(sizeof(QueueNode));
  assert(data);
  new_back->next = NULL;
  new_back->data = data;
  if (!self->back_)
    self->top_ = new_back;
  else
    self->back_->next = new_back;
  self->back_ = new_back;
  atomic_fetch_add(&self->size_, 1);
}

int QueueEmpty(Queue* self) {
  return atomic_load(&self->size_) == 0;
}
