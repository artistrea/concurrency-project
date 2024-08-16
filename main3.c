#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

sem_t king_interrupt_sem;

typedef enum {
  _KING_ACTION_TERMINATOR = 0,
  KING_IDLE,
  KING_TALK_TO_NOBLE,
  KING_END_BALL
} king_action_type_t;

typedef struct king_action {
  king_action_type_t type;
  void *params;
  struct king_action *next;
} king_action_t;

typedef struct {
  int duration;
} king_idle_params_t;

typedef struct {
  int duration;
  int to_noble;
} king_talk_to_noble_params_t;

typedef enum {
  _NOBLE_ACTION_TERMINATOR=0,
  NOBLE_IDLE,
  NOBLE_END_BALL,
  NOBLE_TALK_TO_KING,
  // NOBLE_TALK_TO_NOBLE,
  // NOBLE_DANCE_WITH_NOBLE
} noble_action_type_t;

typedef struct noble_action {
  int priority;
  noble_action_type_t type;
  void *params;
  struct noble_action *next;
} noble_action_t;

typedef king_idle_params_t noble_idle_params_t;

typedef struct {
  pthread_mutex_t lock;
  sem_t alert_important_action_sem;
  noble_action_t* head;
  noble_action_t* tail;
} noble_action_list_t;


typedef struct linked_ints {
  int data;
  struct linked_ints *next;
  struct linked_ints *previous;
} linked_ints_t;

typedef struct int_linked_list {
  linked_ints_t *head;
  linked_ints_t *tail;
} int_linked_list_t;

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t waiting_on_queue_cond;
  pthread_cond_t king_waiting_presence;
  pthread_cond_t noble_waiting_dismissal;
  int selected_noble;
  int all_dismissed;
  int_linked_list_t nobles_waiting;
} king_talk_queue_t;

king_talk_queue_t king_talk_queue = {
  .mutex=PTHREAD_MUTEX_INITIALIZER,
  .waiting_on_queue_cond=PTHREAD_COND_INITIALIZER,
  .king_waiting_presence=PTHREAD_COND_INITIALIZER,
  .selected_noble=-1,
  .all_dismissed=0,
  .nobles_waiting={0}
};

// allocates memory on the heap and sets all parameters (except ->next) to be equal to non_heap_action
// @important: action->next is set to NULL before returning
king_action_t* king_action_heap_alloc(king_action_t* non_heap_action) {
  king_action_t* action = malloc(sizeof(king_action_t));

  action->type = non_heap_action->type;
  // this is evaluated at comptime, so does not have the flexibility we require
  // action->params = malloc(sizeof(non_heap_action->params));

  switch (action->type) {
    case KING_IDLE: {
        action->params = malloc(sizeof(king_idle_params_t));
        break;
      }
    case KING_TALK_TO_NOBLE: {
        action->params = malloc(sizeof(king_idle_params_t));
        break;
      }
    // no params actions:
    case KING_END_BALL:
    case _KING_ACTION_TERMINATOR: {
        break;
      }
  }

  action->params = non_heap_action->params;
  action->next = NULL;

  return action;
}
noble_action_t* noble_action_heap_alloc(noble_action_t* non_heap_action) {
  noble_action_t* action = malloc(sizeof(noble_action_t));

  action->type = non_heap_action->type;
  // // the commented attempt is evaluated at comptime, so does not have the flexibility we require
  // action->params = malloc(sizeof(non_heap_action->params));

  switch (action->type) {
    case NOBLE_IDLE: {
        action->params = malloc(sizeof(noble_idle_params_t));
        break;
      }
    // no params actions:
    case NOBLE_TALK_TO_KING: 
    case NOBLE_END_BALL:
    case _NOBLE_ACTION_TERMINATOR: {
        break;
      }
  }

  action->params = non_heap_action->params;
  action->next = NULL;

  return action;
}

#define N_NOBLES 2
void *noble_action_lists[N_NOBLES]; // convert to noble_action_list_t*

// TODO: dedupe repeated actions?
void king_signal_action(size_t to_noble, noble_action_t* action) {
  action->priority = 999;
  // should enqueue action and signal noble
  // in case noble receives signal while executing another action, he will receive the signal, stop, and do what was asked
  // in case noble receives signal while executing desired action, he will discard the signal
  // he can know if action was already executed cmparing action mem address, &current action == &enqueued action
  noble_action_list_t *action_list = noble_action_lists[to_noble];
  pthread_mutex_lock(
    &action_list->lock
  );
  int should_replace_current_action = 0; // should just enqueue if already executing king priority action
  if (action_list->head->priority < action->priority) {
    should_replace_current_action = 1;
  }
  noble_action_t *should_append_to = action_list->head;

  if (should_append_to == NULL) {
    printf("[UTIL ERROR]: action_list->head is null\n");
  }
  
  while (should_append_to->next != NULL && should_append_to->next->priority >= action->priority) {
    should_append_to = should_append_to->next;
  }
  action->next = should_append_to->next;
  should_append_to->next = action;

  if (should_replace_current_action && should_append_to != NULL) {
    // should signal noble so that he stops current action immediately
    // @important a noble could be signaled even when already completed action. Dedupe action
    sem_post(&action_list->alert_important_action_sem);
  }

  // @important head could be updated right when action just finished. Should be careful to not discard it when noble picks up lock
  pthread_mutex_unlock(
    &action_list->lock
  );
}

void king_broadcast_action(noble_action_t* action) {
  for (size_t i=0;i<N_NOBLES;i++) {
    king_signal_action(i, action);
  }
}

void * king_routine(void* arg) {
  king_action_t* action = arg;

  while (action != NULL) {
    switch (action->type) {
      case _KING_ACTION_TERMINATOR: break; // unreachable code
      case KING_IDLE: {
        king_idle_params_t *params = (king_idle_params_t*)action->params;
        printf("[king] idling for %d seconds\n", params->duration);
        sleep(params->duration);
        break;
      }
      case KING_TALK_TO_NOBLE: {
        king_talk_to_noble_params_t *params = action->params;

        pthread_mutex_lock(&king_talk_queue.mutex);

        if (params->to_noble == -1) {
          if (king_talk_queue.nobles_waiting.head != NULL) {
            params->to_noble = king_talk_queue.nobles_waiting.head->data;
          } else {
            params->to_noble = lrand48()%N_NOBLES;
          }
        }

        king_talk_queue.selected_noble = params->to_noble;


        // since king waits for answer, no need allocate action on the heap
        // when noble acesses action, it will still be within the variable's lifetime
        int already_on_queue = 0;
        linked_ints_t *noble_place = king_talk_queue.nobles_waiting.head;
  
        while (noble_place != NULL) {
          if (noble_place->data == king_talk_queue.selected_noble){
            already_on_queue = 1;
            break;
          }
          noble_place = noble_place->next;
        }
        
        if (!already_on_queue) {
          king_signal_action(
            king_talk_queue.selected_noble,
            noble_action_heap_alloc(&(noble_action_t){
             .type=NOBLE_TALK_TO_KING,
          }));
        } else {
          pthread_cond_broadcast(&king_talk_queue.waiting_on_queue_cond);
        }

        struct timespec sleep_time = {0};
        printf("[king] wants to talk to %02d, he has 1 second to present himself\n", king_talk_queue.selected_noble);
        timespec_get(&sleep_time, TIME_UTC);
        sleep_time.tv_sec += 1;
        int result = pthread_cond_timedwait(&king_talk_queue.king_waiting_presence, &king_talk_queue.mutex, &sleep_time);

        pthread_mutex_unlock(&king_talk_queue.mutex);

        if (result != 0) { // returns -1 if timed out
          printf("ERROR: [king] for some reason noble (%02d) didn't present himself\n", params->to_noble);
        } else {
          printf("[king] noble (%02d) presented himself, dismissing him from the conversation in %d seconds\n", params->to_noble, params->duration);
          sleep(params->duration);
          pthread_mutex_lock(&king_talk_queue.mutex);
          king_talk_queue.selected_noble = -1;

          noble_place = king_talk_queue.nobles_waiting.head;
          while (noble_place != NULL && noble_place->data != params->to_noble) {
            noble_place = noble_place->next;
          }
          if (noble_place != NULL) {
            if (noble_place->previous != NULL) {
              noble_place->previous->next = noble_place->next;
            }
            if (noble_place->next != NULL) {
              noble_place->next->previous = noble_place->previous;
            }
            if (king_talk_queue.nobles_waiting.head == noble_place) {
              king_talk_queue.nobles_waiting.head = noble_place->next;
            }
            if (king_talk_queue.nobles_waiting.tail == noble_place) {
              king_talk_queue.nobles_waiting.tail = noble_place->previous;
            }
            free(noble_place);
          }

          printf("[king] dismissing noble (%02d) \n", params->to_noble);
          pthread_cond_broadcast(&king_talk_queue.noble_waiting_dismissal);
          pthread_mutex_unlock(&king_talk_queue.mutex);
        }


        break;
      }
      case KING_END_BALL: {
        printf("[king] ending ball\n");
        noble_action_t *new_action = malloc(sizeof(noble_action_t));
        new_action->type = NOBLE_END_BALL;
        king_broadcast_action(new_action);

        pthread_mutex_lock(&king_talk_queue.mutex);
          king_talk_queue.all_dismissed = 1;
          pthread_cond_broadcast(&king_talk_queue.waiting_on_queue_cond);
        pthread_mutex_unlock(&king_talk_queue.mutex);
        pthread_exit(0);
        break;
      }
    }

    action = action->next;
  }
  
  pthread_exit(0);
}


int king_spawn_(pthread_t* restrict newthread, king_action_t actions[]) {
  king_action_t* head, *current_ptr, *prev_ptr, current_arg;

  size_t i=0;

  while (actions[i].type != _KING_ACTION_TERMINATOR) {
    current_arg = actions[i];
    prev_ptr = current_ptr;

    // should heap allocate to not invalidate pointer after thread creation
    current_ptr = king_action_heap_alloc(&current_arg);

    if (i == 0) {
      head = current_ptr;
    } else {
      prev_ptr->next = current_ptr;
    }

    i++;
  }

  return pthread_create(newthread, NULL, king_routine, (void*)head);
}

// variadic to null terminated array
#define king_spawn(thread, ...)                                                       \
  king_spawn_(                                                                        \
    thread,                                                                           \
    (king_action_t[]){ __VA_ARGS__, (king_action_t){.type=_KING_ACTION_TERMINATOR} }  \
  )

void * noble_routine(void* arg) {
  int *noble_id = arg;
  noble_action_list_t *actions_list = noble_action_lists[*noble_id];
  noble_action_t *action = actions_list->head;

  while (1) {
    switch (action->type) {
      case _NOBLE_ACTION_TERMINATOR: break; // unreachable code
      case NOBLE_IDLE: {
        noble_idle_params_t *params = (noble_idle_params_t*)action->params;
        printf("[%02d] noble idling for %d seconds\n", *noble_id, params->duration);
        struct timespec sleep_time = {0};
        timespec_get(&sleep_time, TIME_UTC);
        sleep_time.tv_sec += params->duration;
        int result = sem_timedwait(&actions_list->alert_important_action_sem, &sleep_time);
        if (result == 0) { // was warned. would return -1 if timed out
          printf("[%02d] noble received more important orders, idling interrupted\n", *noble_id);
        }
        break;
      }
      case NOBLE_TALK_TO_KING: {
        printf("[%02d] noble entering queue to talk to king\n", *noble_id);
        pthread_mutex_lock(&king_talk_queue.mutex);

        linked_ints_t* next_tail = malloc(sizeof(linked_ints_t));
        next_tail->next = NULL;
        next_tail->previous = NULL;
        next_tail->data = *noble_id;
        if (king_talk_queue.nobles_waiting.tail != NULL) {
          king_talk_queue.nobles_waiting.tail->next = next_tail;
          next_tail->previous = king_talk_queue.nobles_waiting.tail;
        } else { // either both head and tail should be NULL, or neither is
          king_talk_queue.nobles_waiting.head = next_tail;
        }

        king_talk_queue.nobles_waiting.tail = next_tail;
        while (king_talk_queue.selected_noble != (*noble_id) && (!king_talk_queue.all_dismissed)) {
          pthread_cond_wait(&king_talk_queue.waiting_on_queue_cond, &king_talk_queue.mutex);
        }

        if (king_talk_queue.all_dismissed) {
          printf("[%02d] dismissed from talk to king queue\n", *noble_id);
          pthread_mutex_unlock(&king_talk_queue.mutex);
          break;
        }

        printf("[%02d] presenting himself to talk to king\n", *noble_id);

        pthread_cond_signal(&king_talk_queue.king_waiting_presence);
        king_talk_queue.selected_noble = -2;
        while (king_talk_queue.selected_noble == -2) {
          pthread_cond_wait(&king_talk_queue.noble_waiting_dismissal, &king_talk_queue.mutex);
        }

        printf("[%02d] dismissed by king\n", *noble_id);

        pthread_mutex_unlock(&king_talk_queue.mutex);

        break;
      }
      case NOBLE_END_BALL: {
        printf("[%02d] noble leaving ball\n", *noble_id);
        pthread_exit(0);
        break;
      }
    }
    // any time someone inserts action, it can be considered that the action being executed is not 
    pthread_mutex_lock(&actions_list->lock);
    if (actions_list->head == action)
      actions_list->head = actions_list->head->next;
    if (actions_list->head == NULL) {
      // if no more actions left
      actions_list->head = noble_action_heap_alloc(&(noble_action_t){
        .params=&(noble_idle_params_t){
          .duration=100,
        },
        .type=NOBLE_IDLE,
        .priority=0
      });
      action->next = action; // always idle, until king ends
    }

    action = actions_list->head;
    pthread_mutex_unlock(&actions_list->lock);
  }

  pthread_exit(0);
}

int noble_spawn_(pthread_t* restrict newthread, int *noble_id, noble_action_t actions[]) {
  noble_action_t *current_ptr;

  noble_action_lists[*noble_id] = malloc(sizeof(noble_action_list_t));
  noble_action_list_t *actions_list = noble_action_lists[*noble_id];

  pthread_mutex_init(&(actions_list->lock), NULL);
  sem_init(&actions_list->alert_important_action_sem, 0, 0);
  actions_list->head = NULL;
  actions_list->tail = NULL;

  size_t i=0;

  while (actions[i].type != _NOBLE_ACTION_TERMINATOR) {
    // should heap allocate to not invalidate pointers after thread creation
    current_ptr = noble_action_heap_alloc(&actions[i]);

    if (actions_list->head == NULL) {
      actions_list->head = current_ptr;
    } else {
      actions_list->tail->next = current_ptr;
    }

    actions_list->tail = current_ptr;
    i++;
  }
  // return 1;
  return pthread_create(newthread, NULL, noble_routine, (void*)noble_id);
}

// variadic to null terminated array
#define noble_spawn(thread, noble_id, ...)                                                \
  noble_spawn_(                                                                           \
    thread,                                                                               \
    noble_id,                                                                             \
    (noble_action_t[]){ __VA_ARGS__, (noble_action_t){.type=_NOBLE_ACTION_TERMINATOR} }   \
  )


int main() {
  pthread_t king_thread, noble_threads[N_NOBLES];
  // seeding for drand48
  srand48(time(NULL));

  sem_init(&king_interrupt_sem, 0, 0);

  int *id;
  id = malloc(sizeof(int));
  *id = 0;
  noble_spawn(
              &noble_threads[*id],
              id,
              (noble_action_t){
                .type=NOBLE_IDLE,
                .params=&(noble_idle_params_t){
                  .duration=15
                }
              },
              (noble_action_t){
                .type=NOBLE_TALK_TO_KING,
                .priority=999
              }
            );

  id = malloc(sizeof(int));
  *id = 1;
  noble_spawn(
              &noble_threads[*id],
              id,
              (noble_action_t){
                .type=NOBLE_IDLE,
                .params=&(noble_idle_params_t){
                  .duration=1
                }
              },
              (noble_action_t){
                .type=NOBLE_TALK_TO_KING,
                .priority=999
              }
            );

  king_spawn(&king_thread,
     (king_action_t){
       .type=KING_IDLE,
       .params=&(king_idle_params_t){
         .duration=3
       }
     },
     // (king_action_t){
     //   .type=KING_TALK_TO_NOBLE,
     //   .params=&(king_talk_to_noble_params_t){
     //     .duration=3,
     //     .to_noble=-1
     //   }
     // },
     // (king_action_t){
     //   .type=KING_IDLE,
     //   .params=&(king_idle_params_t){
     //     .duration=2
     //   }
     // },
     // (king_action_t){
     //   .type=KING_TALK_TO_NOBLE,
     //   .params=&(king_talk_to_noble_params_t){
     //     .duration=3,
     //     .to_noble=-1
     //   }
     // },
     // (king_action_t){
     //   .type=KING_TALK_TO_NOBLE,
     //   .params=&(king_talk_to_noble_params_t){
     //     .duration=10,
     //     .to_noble=-1
     //   }
     // },
     // (king_action_t){
     //   .type=KING_IDLE,
     //   .params=&(king_idle_params_t){
     //     .duration=0
     //   }
     // },
     (king_action_t){
       .type=KING_IDLE,
       .params=&(king_idle_params_t){
         .duration=5
       }
     },
     (king_action_t){
       .type=KING_END_BALL
     }
   );

  pthread_join(king_thread, NULL);

  for (int i = 0; i < N_NOBLES; i++) {
    pthread_join(noble_threads[i], NULL);
  }
  
  return 0;
}

