#ifndef NOBLE_H
#define NOBLE_H

#include <pthread.h>
#include <semaphore.h>
#include "king.h"


typedef enum {
  // just a terminator for the variadic
  _NOBLE_ACTION_TERMINATOR=0,

  NOBLE_IDLE,
  NOBLE_END_BALL,
  NOBLE_TALK_TO_KING,
  NOBLE_TALK_TO_NOBLE,
} noble_action_type;

typedef struct noble_action {
  int priority;
  noble_action_type type;
  void *params;
  struct noble_action *next;
} noble_action;

typedef struct {
  int duration;
} noble_idle_params;

typedef struct {
  int to_noble;
  int duration_in_seconds;
  int can_wait_in_seconds;
} noble_talk_to_noble_params;

typedef struct {
  size_t id;
  pthread_mutex_t lock;
  sem_t alert_important_action_sem;
  noble_action* head;
  noble_action* tail;
} noble_action_list;

noble_action_list* define_noble_actions_(size_t n, noble_action actions[]);

#ifndef NUMARGS
#define NUMARGS(type, ...) (size_t)(sizeof((type[]){__VA_ARGS__})/sizeof(type))
#endif

#define define_noble_actions(...)\
  define_noble_actions_(NUMARGS(noble_action,  __VA_ARGS__), (noble_action[]){ __VA_ARGS__ })

void * noble_routine(void * action_list);
  
#endif // NOBLE_H

