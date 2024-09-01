#ifndef KING_H
#define KING_H

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

typedef enum {
  _KING_ACTION_TERMINATOR = 0,
  KING_IDLE,
  KING_TALK_TO_NOBLE,
  KING_END_BALL
} king_action_type;

typedef struct king_action_struct {
  king_action_type type;
  void *params;
  struct king_action_struct *next;
} king_action;

typedef struct {
  int duration;
} king_idle_params;

typedef struct {
  int duration;
  int to_noble;
} king_talk_to_noble_params;

#ifndef NUMARGS
#define NUMARGS(type, ...) (size_t)(sizeof((type[]){__VA_ARGS__})/sizeof(type))
#endif
void * king_routine(void* arg);


// CLIENT EXPOSED FUNCTIONS:

king_action* define_king_actions_(size_t n_actions, king_action king_actions[]);

#define define_king_actions(...) define_king_actions_(NUMARGS(king_action, __VA_ARGS__), (king_action[]){__VA_ARGS__})

#endif

