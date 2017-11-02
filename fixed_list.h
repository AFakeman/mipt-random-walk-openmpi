#include <stddef.h>

// A linked list structure that occupies fixed memory
// and only allocates memory on creation.
struct FixedList;

typedef struct FixedListNode {
  struct FixedListNode* next;
  void* data;
} FixedListNode;

typedef struct FixedList FixedList;

// Create a fixed list of |size| elements.
FixedList* FixedListCreate(size_t size);

void FixedListDelete(FixedList* self);

// Push |data| into the list. The list does not assume
// ownership over it.
void FixedListPushFront(FixedList* self, void* data);

// Delete the element that comes after |prev|.
// To delete the first one, pass |NULL|.
void FixedListDeleteElement(FixedList* self, FixedListNode* prev);

size_t FixedListSize(const FixedList* self);

FixedListNode* FixedListBegin(FixedList* self);