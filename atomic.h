#include <pthread.h>
#include <stddef.h>

#pragma once

struct atomic_size_t {
	pthread_mutex_t mtx;
	size_t value;
};

typedef struct atomic_size_t atomic_size_t;

void atomic_init(atomic_size_t* self);

void atomic_destroy(atomic_size_t* self);

size_t atomic_load(atomic_size_t *self);

void atomic_store(atomic_size_t *self, size_t value);

size_t atomic_fetch_add(atomic_size_t *self, size_t value);

size_t atomic_fetch_sub(atomic_size_t *self, size_t value);

size_t atomic_exchange(atomic_size_t *self, size_t value);