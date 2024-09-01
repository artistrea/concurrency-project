#ifndef EVENTS_H
#define EVENTS_H
// WARNING: this code is supposed to be free from business logic
// call abstractionsm from ball.h to be happy

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

typedef struct {
  void (*fn)(void*);
} event_handler;

typedef struct {
  size_t id;
  uint32_t type;
  void* params; // should be acoording to event type
  void* curried_params; // should be acoording to registered listener
} event;

int emit_event(event ev); 

typedef struct {
  void (*fn)(event); // receives event
  void* curried_values; // use if needed
} event_listener;

// can register up to M listeners
// WARNING: blocks execution
uint32_t register_event_listener(event_listener listener);

void unregister_event_listener(uint32_t id);

typedef struct {
  uint32_t events_mask;
} event_mask_evaluator_params;

typedef struct {
  sem_t sem;
  int (*evaluator)(event);
  void *evaluator_curried_params;
  int event_listener_id;
} wait_for_event_evaluated_params;

typedef struct {
  event ev;
  int wait_result;
  int found_result;
} wait_for_event_result;

// to be used as wait_for_event(setup_wait_for_event(...));

wait_for_event_evaluated_params* setup_wait_for_event_evaluation(int (*evaluator)(event ev), void* evaluator_curried_params);

wait_for_event_evaluated_params* setup_wait_for_event_mask(uint32_t events_mask);

wait_for_event_result* wait_for_event(wait_for_event_evaluated_params *setup);

wait_for_event_result* timedwait_for_event(wait_for_event_evaluated_params *setup, struct timespec *sleep_time);

// @important: there is no way to kill the events thread as of now
int events_manager_thread_create(pthread_t *neweventsmanagerthread);

void events_manager_setup();

#endif // EVENTS_H

