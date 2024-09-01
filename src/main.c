// #include "events.h"
#include "king.h"
#include "noble.h"
#include "ball.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

void render_fn(char *king_text, size_t n_nobles, char *noble_texts[]) {
  // non portable POSIX clear screen:
  printf("\e[1;1H\e[2J");

  printf("[KING] %s\n", king_text);
  for (size_t i=0; i<n_nobles; i++) {
    printf("[N %02lu] %s\n", i, noble_texts[i]);
  }
}

// void listener_test(event ev) {
//   ball_set_king_text("TESTE");
// }

int main() {
  srand48(time(NULL));

  // unregister_event_listener(
  //   register_event_listener((event_listener){ .fn=&listener_test })
  // );
  // ncurses render setup
  create_ball(
    &render_fn,
    define_king_actions(
      (king_action){
        .type=KING_IDLE,
        .params=&(king_idle_params){
          .duration=2
        }
      },
      (king_action){
        .type=KING_IDLE,
        .params=&(king_idle_params){
          .duration=1
        }
      },
      (king_action){
        .type=KING_TALK_TO_NOBLE,
        .params=&(king_talk_to_noble_params){
          .duration=5,
          .to_noble=0
        }
      },
      (king_action){
        .type=KING_TALK_TO_NOBLE,
        .params=&(king_talk_to_noble_params){
          .duration=6,
          .to_noble=1
        }
      },
      (king_action){
        .type=KING_TALK_TO_NOBLE,
        .params=&(king_talk_to_noble_params){
          .duration=4,
          .to_noble=-1
        }
      },
      (king_action){
        .type=KING_END_BALL
      }
    ),
    define_noble_actions(
      (noble_action){
        .type=NOBLE_TALK_TO_KING
      },
      (noble_action){
        .type=NOBLE_IDLE,
        .params=&(noble_idle_params){
          .duration=7
        }
      },
      (noble_action){
        .type=NOBLE_IDLE,
        .params=&(noble_idle_params){
          .duration=3
        }
      }
    ),
    define_noble_actions(
      (noble_action){
        .type=NOBLE_TALK_TO_KING
      },
      (noble_action){
        .type=NOBLE_IDLE,
        .params=&(noble_idle_params){
          .duration=1
        }
      },
      (noble_action){
        .type=NOBLE_IDLE,
        .params=&(noble_idle_params){
          .duration=2
        }
      }
    )
  );

  return 0;
}

