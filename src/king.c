#include "king.h"
#include "ball.h"
#include "events.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

king_action* define_king_actions_(size_t n_actions, king_action king_actions[]) {
  king_action* king_actions_mem = (king_action*)malloc(n_actions *  sizeof(king_action));

  for (size_t i=0; i<n_actions; i++) {
    king_actions_mem[i] = king_actions[i];

    // @important overwrite pointers to new heap allocated mem
    if (i == n_actions-1) {
      king_actions_mem[i].next = NULL;
    } else {
      king_actions_mem[i].next = &king_actions_mem[i+1];
    }

    switch (king_actions_mem[i].type) {
      case KING_IDLE: {
          // could accumulate to also put in vector (pointer of max(sizeof(each params)))
          // better malloc perf
          king_actions_mem[i].params = malloc(sizeof(king_idle_params));
          memcpy(king_actions_mem[i].params, king_actions[i].params, sizeof(king_idle_params));
          break;
        }
      case KING_TALK_TO_NOBLE: {
          king_actions_mem[i].params = malloc(sizeof(king_talk_to_noble_params));
          memcpy(king_actions_mem[i].params, king_actions[i].params, sizeof(king_talk_to_noble_params));
          break;
        }
      // no params actions:
      case KING_END_BALL:
      case _KING_ACTION_TERMINATOR: {
          break;
        }
    }
  }

  return king_actions_mem;
}

void * king_routine(void* arg) {
  king_action* action = arg;
  // ball_set_king_text("king routine\n");
  while (action != NULL) {
    switch (action->type) {
      case _KING_ACTION_TERMINATOR: break; // unreachable code
      case KING_IDLE: {
        king_idle_params *params = (king_idle_params*)action->params;
        ball_set_king_text("idling for %d seconds\n", params->duration);
        sleep(params->duration);
        break;
      }
      case KING_TALK_TO_NOBLE: {
        king_talk_to_noble_params *params = action->params;


        params->to_noble = request_noble_to_talk_to_king(params->to_noble);

        ball_set_king_text("wants to talk to %02d, he has 1 second to present himself\n", params->to_noble);

        struct timespec sleep_time = {0};
        timespec_get(&sleep_time, TIME_UTC);
        sleep_time.tv_sec += 1;

        wait_for_event_evaluated_params* setup_wait = setup_wait_for_event_mask(NOBLE_EMITTED_PRESENTED_HIMSELF_TO_KING);
        
        emit_event(
          (event){
            .type=KING_EMITTED_NEXT_ON_QUEUE
          }
        );

        wait_for_event_result* result = timedwait_for_event(setup_wait, &sleep_time);

        if (result->wait_result != 0) { // returns -1 if timed out
          ball_set_king_text("ERROR: for some reason noble (%02d) didn't present himself\n", params->to_noble);
        } else {
          ball_set_king_text("noble (%02d) presented himself, dismissing him from the conversation in %d seconds\n", params->to_noble, params->duration);

          sleep(params->duration);

          emit_event(
            (event){
              .type=KING_EMITTED_DISMISS_NOBLE_TALKING
            }
          );

          ball_set_king_text("dismissing noble (%02d) \n", params->to_noble);
        }

        break;
      }
      case KING_END_BALL: {
        ball_set_king_text("ending ball\n");

        pthread_mutex_lock(&ball_is_over_info.mutex);
        ball_is_over_info.is_over = 1;
        pthread_mutex_unlock(&ball_is_over_info.mutex);

        emit_event((event){ .type=KING_EMITTED_BALL_IS_OVER });

        pthread_exit(0);
        break;
      }
    }

    action = action->next;
  }
  
  pthread_exit(0);
}

