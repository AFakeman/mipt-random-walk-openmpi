#include "fixed_list.h"

#include <stdlib.h>

struct FixedList {
  FixedListNode* free_top_;
  FixedListNode* top_;
  FixedListNode* pool_;
  size_t pool_size_;
  size_t size_;
};

FixedList* FixedListCreate(size_t size) {
  FixedList* self = (FixedList*)malloc(sizeof(FixedList));
  self->pool_ = (FixedListNode*)malloc(size * sizeof(FixedList));
  for (size_t i = 0; i < size - 1; ++i) {
    self->pool_[i].next = self->pool_ + i + 1;
  }
  self->pool_[size - 1].next = NULL;
  self->pool_size_ = size;
  self->size_ = 0;
  self->top_ = NULL;
  self->free_top_ = self->pool_;
  return self;
}

void FixedListDelete(FixedList* self) {
  free(self->pool_);
  free(self);
}

FixedListNode* StackPop(FixedListNode** stk) {
  FixedListNode* to_return = *stk;
  *stk = to_return->next;
  return to_return;
}

void StackPush(FixedListNode** stk, FixedListNode* new) {
  new->next = *stk;
  *stk = new;
}

void FixedListPushFront(FixedList* self, void* data) {
  FixedListNode* new_top = StackPop(&self->free_top_);
  new_top->data = data;
  ++self->size_;
  StackPush(&self->top_, new_top);
}

void FixedListDeleteElement(FixedList* self, FixedListNode* prev) {
  if (!prev) {
    FixedListNode* new_free_top = StackPop(&self->top_);
    StackPush(&self->free_top_, new_free_top);
  } else if (prev->next) {
    FixedListNode* new_free_top = prev->next;
    prev->next = prev->next->next;
    StackPush(&self->free_top_, new_free_top);
  }
  --self->size_;
}

FixedListNode* FixedListBegin(FixedList* self) {
  return self->top_;
}

size_t FixedListSize(const FixedList* self) {
  return self->size_;
}