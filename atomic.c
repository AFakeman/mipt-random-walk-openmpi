#include "atomic.h"

void atomic_init(atomic_size_t* self) {
	pthread_mutex_init(&self->mtx, NULL);
}

void atomic_destroy(atomic_size_t* self) {
	pthread_mutex_destroy(&self->mtx);
}

size_t atomic_load(atomic_size_t *self) {
	pthread_mutex_lock(&self->mtx);
	size_t result = self->value;
	pthread_mutex_unlock(&self->mtx);
	return result;
}

void atomic_store(atomic_size_t *self, size_t value) {
	pthread_mutex_lock(&self->mtx);
	self->value = value;
	pthread_mutex_unlock(&self->mtx);
}

size_t atomic_fetch_add(atomic_size_t *self, size_t value) {
	pthread_mutex_lock(&self->mtx);
	size_t result = self->value;
	self->value += value;
	pthread_mutex_unlock(&self->mtx);
	return result;
}

size_t atomic_fetch_sub(atomic_size_t *self, size_t value) {
	pthread_mutex_lock(&self->mtx);
	size_t result = self->value;
	self->value -= value;
	pthread_mutex_unlock(&self->mtx);
	return result;
}

size_t atomic_exchange(atomic_size_t *self, size_t value) {
	pthread_mutex_lock(&self->mtx);
	size_t result = self->value;
	self->value = value;
	pthread_mutex_unlock(&self->mtx);
	return result;
}