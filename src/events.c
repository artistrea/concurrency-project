#include "recursive_mutex.h"
#include "events.h"

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define N 256

struct {
  size_t using;
  size_t end;
  event buffer[N];
} events_buffer = {
  .using=0,
  .end=0
};

typedef struct linked_event_listener_struct {
  int id;
  event_listener listener;
  struct linked_event_listener_struct *next;
  struct linked_event_listener_struct *prev;
} linked_event_listener;


typedef struct {
  linked_event_listener *head;
  linked_event_listener *tail;
  recursive_mutex mutex;
} all_event_listeners;

typedef struct event_listener_results_struct {
  void *res;
  uint32_t event_listener_id;
  struct event_listener_results_struct* next;
  struct event_listener_results_struct* prev;
} event_listener_results;

event_listener_results *results = NULL;

pthread_mutex_t results_lock = PTHREAD_MUTEX_INITIALIZER;

void* get_event_listener_result(uint32_t listener_id) {
  pthread_mutex_lock(&results_lock);
  event_listener_results* res = results;

  while (res != NULL && res->event_listener_id != listener_id) {
    if (res == res->next) {
      printf("\n\n#### ERROR: res == res->next\n\n");
      break;
    }
    res = res->next;
  }

  if (res == NULL) {
    pthread_mutex_unlock(&results_lock);

    return NULL;
  }

  // if (res == results) {
  //   results = res->next;
  // }

  // if (res->next != NULL) {
  //   res->next->prev = res->prev;
  // }

  // if (res->prev != NULL) {
  //   res->prev->next = res->next;
  // }

  void *actual_res = res->res;
  // free(res);

  pthread_mutex_unlock(&results_lock);

  return actual_res;
}

void add_event_listener_result(uint32_t listener_id, void* result) {
  pthread_mutex_lock(&results_lock);

  event_listener_results* substitute = results;

  while (substitute != NULL && substitute->event_listener_id != listener_id) {
    if (substitute == substitute->next) {
      printf("\n\n#### ERROR: substitute == substitute->next\n\n");
      break;
    }
    substitute = substitute->next;
  }

  if (substitute == NULL) {
    event_listener_results* to_add = malloc(sizeof(event_listener_results));

    to_add->event_listener_id = listener_id;
    to_add->res = result;
    to_add->prev = NULL;
    to_add->next = results;

    if (results != NULL) {
      results->prev = to_add;
    }

    results = to_add;
  } else {
    substitute->res = result;
  }

  pthread_mutex_unlock(&results_lock);
}


all_event_listeners all_listeners = {
  .head=NULL,
  .tail=NULL,
  .mutex=RECURSIVE_MUTEX_INITIALIZER
};

pthread_cond_t new_event_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t event_id = 0; // autoincrement
size_t event_listener_id = 0; // autoincrement

int event_mask_evaluator(event ev) {
  event_mask_evaluator_params *params = ((wait_for_event_evaluated_params*)ev.curried_params)->evaluator_curried_params;

  return params->events_mask & ev.type;
}

void listener_wait_for_event(event ev) {
  wait_for_event_evaluated_params *params = ev.curried_params;

  if (params->evaluator(ev)) {
    unregister_event_listener(params->event_listener_id);
    // free(params);
    wait_for_event_result *result = malloc(sizeof(wait_for_event_result));
    // TODO: check if other params need to be malloc'd/are freed elsewhere
    result->ev = ev;
    add_event_listener_result(params->event_listener_id, result);

    sem_post(&params->sem);
    // free(params);
  }
}

wait_for_event_evaluated_params* setup_wait_for_event_mask(uint32_t events_mask) {
  event_mask_evaluator_params *mask_evaluator_params = malloc(sizeof(event_mask_evaluator_params));
  mask_evaluator_params->events_mask = events_mask;

  return setup_wait_for_event_evaluation(&event_mask_evaluator, mask_evaluator_params);
}

wait_for_event_evaluated_params* setup_wait_for_event_evaluation(int (*evaluator)(event ev), void* evaluator_curried_params) {
  wait_for_event_evaluated_params *curried_params = malloc(sizeof(wait_for_event_evaluated_params));
  curried_params->evaluator_curried_params = evaluator_curried_params;
  curried_params->evaluator = evaluator;
  sem_init(&curried_params->sem, 0, 0);

  // @important: lock here to prevent race condition
  recursive_mutex_lock(&all_listeners.mutex);
  uint32_t listener_id = register_event_listener((event_listener){ .fn=&listener_wait_for_event, .curried_values=curried_params });
  curried_params->event_listener_id = listener_id;
  recursive_mutex_unlock(&all_listeners.mutex);

  return curried_params;
}

// this function checks if (event_mask & event_type) to trigger
wait_for_event_result* wait_for_event(wait_for_event_evaluated_params *setup) {
  return timedwait_for_event(setup, NULL);
}

// this function checks if (event_mask & event_type) to trigger
wait_for_event_result* timedwait_for_event(wait_for_event_evaluated_params *setup, struct timespec *sleep_time) {
  int wait_res;
  if (sleep_time == NULL) {
    wait_res = sem_wait(&setup->sem);
  } else {
    wait_res = sem_timedwait(&setup->sem, sleep_time);
  }

  wait_for_event_result* res = get_event_listener_result(setup->event_listener_id);

  if (res == NULL) {
    res = malloc(sizeof(wait_for_event_result));
    res->found_result = 0;
  } else {
    res->found_result = 1;
  }

  res->wait_result = wait_res;

  return res;
}

// returns < 0 if error
// else returns the event id
// the id will always be set internally
int emit_event(event ev) {
  pthread_mutex_lock(&event_queue_mutex);

  if (events_buffer.using == N) {
    pthread_mutex_unlock(&event_queue_mutex);
    return -1;
  }
  if (events_buffer.using == 0) {
    pthread_cond_signal(&new_event_cond);
  }
  ev.id = event_id++;
  events_buffer.buffer[events_buffer.end++] = ev;
  if (events_buffer.end == N) events_buffer.end = 0;
  events_buffer.using++;

  pthread_mutex_unlock(&event_queue_mutex);

  return 0;
}


// can register up to M listeners
// blocks
uint32_t register_event_listener(event_listener listener) {
  recursive_mutex_lock(&all_listeners.mutex);

  linked_event_listener *new_listener = malloc(sizeof(linked_event_listener));
  new_listener->id = event_listener_id++;
  new_listener->listener = listener;
  new_listener->next = NULL;
  new_listener->prev = all_listeners.tail;
  all_listeners.tail = new_listener;
  if (all_listeners.head == NULL) {
    all_listeners.head = all_listeners.tail;
  } else {
    all_listeners.tail->prev->next = all_listeners.tail;
  }

  recursive_mutex_unlock(&all_listeners.mutex);

  return new_listener->id;
}

void unregister_event_listener(uint32_t id) {
  recursive_mutex_lock(&all_listeners.mutex);

  linked_event_listener *cur_listener = all_listeners.head;
  while (cur_listener != NULL && cur_listener->id != id) {
    cur_listener = cur_listener->next;
  }

  if (cur_listener != NULL) {
    if (cur_listener == all_listeners.head) {
      all_listeners.head = all_listeners.head->next;
    }

    if (cur_listener == all_listeners.tail) {
      all_listeners.tail = all_listeners.tail->prev;
    }

    if (cur_listener->prev != NULL) {
      cur_listener->prev->next = cur_listener->next;
    }
    if (cur_listener->next != NULL) {
      cur_listener->next->prev = cur_listener->prev;
    }

    // free(cur_listener);
  }

  recursive_mutex_unlock(&all_listeners.mutex);
}

void * events_manager_routine() {
  event ev;

  while (1) {
    pthread_mutex_lock(&event_queue_mutex);
    while (events_buffer.using == 0) {
      pthread_cond_wait(&new_event_cond, &event_queue_mutex);
    }

    size_t i = (N + events_buffer.end - events_buffer.using) % N;
    events_buffer.using--;

    ev = events_buffer.buffer[i];
    pthread_mutex_unlock(&event_queue_mutex);

    recursive_mutex_lock(&all_listeners.mutex);

    linked_event_listener *cur_listener = all_listeners.head;
    while (cur_listener != NULL) {
      // TODO: create new thread for each function, so that there is no implicit blocking
      // @important: if the above todo is implemented, should probably malloc event cause it would not be passed by value
      ev.curried_params = cur_listener->listener.curried_values;
      cur_listener->listener.fn(ev);

      if (cur_listener == cur_listener->next) {
        printf("\n\n### ERROR: cur_listener == cur_listener->next\n\n");
      }
      cur_listener = cur_listener->next;
    }

    recursive_mutex_unlock(&all_listeners.mutex);
  }

  pthread_exit(0);
}

void events_manager_setup() {
}

// @important: there is no way to kill the events thread as of now
int events_manager_thread_create(pthread_t *restrict neweventsmanagerthread) {
  return pthread_create(neweventsmanagerthread, NULL, events_manager_routine, NULL);
}

