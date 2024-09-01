#include "noble.h"
#include "ball.h"
#include "events.h"

#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

noble_action* noble_action_heap_alloc(noble_action* non_heap_action) {
  noble_action* action = malloc(sizeof(noble_action));

  action->type = non_heap_action->type;
  // // the commented attempt is evaluated at comptime, so does not have the flexibility we require
  // action->params = malloc(sizeof(non_heap_action->params));

  switch (action->type) {
    case NOBLE_IDLE: {
        action->params = malloc(sizeof(noble_idle_params));
        memcpy(action->params, non_heap_action->params, sizeof(noble_idle_params));
        break;
      }
    case NOBLE_TALK_TO_NOBLE: {
        action->params = malloc(sizeof(noble_talk_to_noble_params));
        memcpy(action->params, non_heap_action->params, sizeof(noble_talk_to_noble_params));
        break;
      }
    // no params actions:
    case NOBLE_TALK_TO_KING: 
    case NOBLE_END_BALL:
    case _NOBLE_ACTION_TERMINATOR: {
        break;
      }
  }

  action->next = NULL;

  return action;
}


noble_action_list* define_noble_actions_(size_t n, noble_action actions[]) {
  noble_action *heap_actions = (noble_action*)malloc(n * sizeof(noble_action));
  noble_action_list* list = (noble_action_list*)malloc(sizeof(noble_action_list));
  
  pthread_mutex_init(&list->lock, NULL);

  list->head = &heap_actions[0];
  list->tail = &heap_actions[n-1];

  for (size_t i=0; i<n; i++) {
    heap_actions[i] = actions[i];
    if (i < n-1) {
      heap_actions[i].next = &heap_actions[i+1];
    } else {
      heap_actions[i].next = NULL;
    }

    switch (heap_actions[i].type) {
      case NOBLE_IDLE: {
          heap_actions[i].params = malloc(sizeof(noble_idle_params));
          memcpy(heap_actions[i].params, actions[i].params, sizeof(noble_idle_params));
          break;
        }
      case NOBLE_TALK_TO_NOBLE: {
          heap_actions[i].params = malloc(sizeof(noble_talk_to_noble_params));
          memcpy(heap_actions[i].params, actions[i].params, sizeof(noble_talk_to_noble_params));
          break;
        }
      // no params actions:
      case NOBLE_TALK_TO_KING: 
      case NOBLE_END_BALL:
      case _NOBLE_ACTION_TERMINATOR: {
          break;
        }
    }
  }

  return list;
}

typedef struct {
  size_t noble_id;
} noble_idle_event_listener_evaluator_params;

int noble_idle_event_listener_evaluator(event ev) {
  if (ev.type & KING_EMITTED_BALL_IS_OVER) return 1;

  if (ev.type & KING_EMITTED_NEXT_ON_QUEUE) {
    noble_idle_event_listener_evaluator_params* params = ((wait_for_event_evaluated_params*)ev.curried_params)->evaluator_curried_params;
    pthread_mutex_lock(&talk_to_king_queue_info.mutex);
    if (talk_to_king_queue_info.king_called_for == params->noble_id) {
      pthread_mutex_unlock(&talk_to_king_queue_info.mutex);
      return 1;
    }
    pthread_mutex_unlock(&talk_to_king_queue_info.mutex);
  }

  return 0;
}

void * noble_routine(void* arg) {
  noble_action_list *actions_list = arg;
  noble_action *action = actions_list->head;
  noble_idle_event_listener_evaluator_params this_noble_idle_event_listener_evaluator_params = {
    .noble_id=actions_list->id
  };

  while (1) {
    switch (action->type) {
      case _NOBLE_ACTION_TERMINATOR: break; // unreachable code
      case NOBLE_IDLE: {
        noble_idle_params *params = (noble_idle_params*)action->params;
        ball_set_noble_text(actions_list->id, "noble idling for %d seconds\n", params->duration);
        struct timespec sleep_time = {0};
        timespec_get(&sleep_time, TIME_UTC);
        sleep_time.tv_sec += params->duration;

        wait_for_event_evaluated_params *setup_wait = setup_wait_for_event_evaluation(
          &noble_idle_event_listener_evaluator,
          &this_noble_idle_event_listener_evaluator_params
        );

        pthread_mutex_lock(&ball_is_over_info.mutex);

        if (ball_is_over_info.is_over) {
          action->type = NOBLE_END_BALL;
          pthread_mutex_unlock(&ball_is_over_info.mutex);
          continue; // goto next action immediately
        }

        pthread_mutex_unlock(&ball_is_over_info.mutex);

        pthread_mutex_lock(&talk_to_king_queue_info.mutex);
        if (talk_to_king_queue_info.king_called_for == actions_list->id) {
          action->type = NOBLE_TALK_TO_KING;
          pthread_mutex_unlock(&talk_to_king_queue_info.mutex);
          continue; // goto next action immediately
        }

        pthread_mutex_unlock(&talk_to_king_queue_info.mutex);

        wait_for_event_result* result = timedwait_for_event(setup_wait, &sleep_time);

        if (result->wait_result == 0) { // was warned. would return -1 if timed out
          if (result->ev.type == KING_EMITTED_BALL_IS_OVER) {
            ball_set_noble_text(actions_list->id, "idling interrupted because ball is over");

            action->type = NOBLE_END_BALL;
          } else if (result->ev.type == KING_EMITTED_NEXT_ON_QUEUE) {
            ball_set_noble_text(actions_list->id, "idling interrupted because king wants to talk to him");

            action->type = NOBLE_TALK_TO_KING;
          }
          continue;
        }
        break;
      }
      case NOBLE_TALK_TO_KING: {
        ball_set_noble_text(actions_list->id, "noble entering queue to talk to king\n");

        enter_talk_to_king_queue(actions_list->id);

        // @important: set this up **BEFORE** the while() to prevent race conditions
        wait_for_event_evaluated_params *setup_wait = setup_wait_for_event_evaluation(
          &noble_idle_event_listener_evaluator,
          &this_noble_idle_event_listener_evaluator_params
        );

        pthread_mutex_lock(&ball_is_over_info.mutex);

        if (ball_is_over_info.is_over) {
          action->type = NOBLE_END_BALL;
          pthread_mutex_unlock(&ball_is_over_info.mutex);
          continue; // goto next action immediately
        }

        pthread_mutex_unlock(&ball_is_over_info.mutex);

        pthread_mutex_lock(&talk_to_king_queue_info.mutex);

        if (talk_to_king_queue_info.king_called_for != actions_list->id) {
          pthread_mutex_unlock(&talk_to_king_queue_info.mutex);

          wait_for_event_result* result = wait_for_event(setup_wait);

          pthread_mutex_lock(&talk_to_king_queue_info.mutex);
        }
        pthread_mutex_unlock(&talk_to_king_queue_info.mutex);

        leave_talk_to_king_queue(actions_list->id);

        pthread_mutex_lock(&ball_is_over_info.mutex);

        if (ball_is_over_info.is_over) {
          action->type = NOBLE_END_BALL;
          pthread_mutex_unlock(&ball_is_over_info.mutex);
          continue; // goto next action immediately
        }

        pthread_mutex_unlock(&ball_is_over_info.mutex);

        ball_set_noble_text(actions_list->id, "presenting himself to talk to king");

        // if king is talking to me, no need to check if ball is over
        // @important: setup wait before emitting event to prevent race condition
        wait_for_event_evaluated_params* setup_wait2 = setup_wait_for_event_mask(KING_EMITTED_DISMISS_NOBLE_TALKING);

        emit_event((event){
          .type=NOBLE_EMITTED_PRESENTED_HIMSELF_TO_KING
        });

        wait_for_event(setup_wait2);

        ball_set_noble_text(actions_list->id, "dismissed by king");

        break;
      }
      // case NOBLE_TALK_TO_NOBLE: {
      //   noble_talk_to_noble_params *params = action->params;
      //   printf("[%02d] noble may wait for up to %d s to talk to noble (%02d)\n", *noble_id, params->can_wait_in_seconds, params->to_noble == (*noble_id) ? params->from_noble : params->to_noble);
      //   pthread_cond *own_condition, *other_noble_condition;
      //   if (params->to_noble == *noble_id) {
      //     own_condition = params->alert_from_noble_cond;
      //     other_noble_condition = params->alert_to_noble_cond;
      //   } else {
      //     own_condition = params->alert_to_noble_cond;
      //     other_noble_condition = params->alert_from_noble_cond;
      //   }
      //   // wait for wait time
      //   pthread_mutex_lock(params->mutex);
      //   if (params->from_noble == *noble_id) {
      //     // TODO: use signal_action
      //     // king_signal_action
      //     noble_action_list *other_n_actionlist = noble_action_lists[params->to_noble];
      //     pthread_mutex_lock(&other_n_actionlist->lock);
      //     noble_action *other_noble_action = noble_action_heap_alloc(action);
      //     other_noble_action->next = other_n_actionlist->head->next;
      //     other_n_actionlist->head->next = other_noble_action;
      //     pthread_mutex_unlock(&other_n_actionlist->lock);
      //   }

      //   if (*params->should_leave) {
      //     printf("[%02d] (%02d) has already left, so abandoning talk to him action\n", *noble_id, params->to_noble);
      //     pthread_mutex_unlock(params->mutex);
      //     break;
      //   }
      //   struct timespec sleep_time = {0};
      //   timespec_get(&sleep_time, TIME_UTC);
      //   sleep_time.tv_sec += params->can_wait_in_seconds;
      //   if (params->to_noble == *noble_id) {
      //     pthread_cond_broadcast(other_noble_condition);
      //   }
      //   int has_timedout = pthread_cond_timedwait(own_condition, params->mutex, &sleep_time);
      //   if (params->from_noble == *noble_id) {
      //     pthread_cond_broadcast(other_noble_condition);
      //   }
      //   if (has_timedout || *params->should_leave) {
      //     printf("%d; %d", has_timedout, *params->should_leave);
      //     *params->should_leave = 1;
      //     printf("[%02d] noble stopped waiting to talk to noble (%02d)\n", *noble_id, params->to_noble == (*noble_id) ? params->from_noble : params->to_noble);
      //     pthread_cond_broadcast(other_noble_condition);
      //     pthread_mutex_unlock(params->mutex);
      //     break;
      //   }

      //   pthread_mutex_unlock(params->mutex);
        
      //   printf("[%02d] talking to (%02d) for %d seconds\n", *noble_id, params->to_noble == (*noble_id) ? params->from_noble : params->to_noble, params->duration_in_seconds);
      //   timespec_get(&sleep_time, TIME_UTC);
      //   sleep_time.tv_sec += params->duration_in_seconds;
      //   int result = sem_timedwait(&actions_list->alert_important_action_sem, &sleep_time);
      //   if (result == 0) { // was warned. would return -1 if timed out
      //     printf("[%02d] noble received more important orders, talking interrupted\n", *noble_id);
      //     // should warn colleague:
      //     if (!*params->should_leave) {
      //       noble_action_list *other_n_actionlist = noble_action_lists[params->to_noble == (*noble_id) ? params->from_noble : params->to_noble];
      //       pthread_mutex_lock(&other_n_actionlist->lock);
      //       // TODO: update action list from sem to condition, cause this may cause bugs (2 sem_posts)
      //       sem_post(&other_n_actionlist->alert_important_action_sem);
      //       pthread_mutex_unlock(&other_n_actionlist->lock);
      //     }
      //     *params->should_leave = 1;
      //   }

      //   break;
      // }
      case NOBLE_END_BALL: {
        ball_set_noble_text(actions_list->id, "noble leaving ball\n");
        pthread_exit(0);
        break;
      }
    }
    // any time someone inserts action, it can be considered that the action being executed is not 
    pthread_mutex_lock(&actions_list->lock);
    actions_list->head = actions_list->head->next;
    if (actions_list->head == NULL) {
      // // if no more actions left
      actions_list->head = noble_action_heap_alloc(&(noble_action){
        .params=&(noble_idle_params){
          .duration=100,
        },
        .type=NOBLE_IDLE
      });
    }

    action = actions_list->head;
    pthread_mutex_unlock(&actions_list->lock);
  }

  pthread_exit(0);
}

