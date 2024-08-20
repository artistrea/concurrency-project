# Noble Ball

The objective is to explore concurrency.

A noble ball will be implemented with the following specs:

1. The nobles and king arrive with a predetermined list of actions to do;

2. The king has the following actions/states:
    * Talk to a noble **(DONE)**
      - He may summon any noble `M`;
      - If he doesn't specify a noble (`M=-1`), he chooses the next noble waiting on queue;
      - If he doesn't specify a noble and there are no nobles on queue, he summons a random noble;
      - The nobles have at most 1 second to present themselves;
    * Idle **(DONE)**
      - King is simply idle;
    * End ball **(DONE)**
      - Should end program;

3. A noble has the following actions/states and reactions:
    * Talk to the king **(DONE)**
      - He enters a queue to talk to the king;
      - If king selected him to talk to, he presents himself to the king;
      - Waits for king dismissal;
      - Reactions:
        * If waiting on the queue and `[KING END BALL]`, then `[NOBLE END BALL]`
        * If waiting on the queue and `[KING SUMMONS]` him occurs or already occurred, then present himself to king;
    * Talk to another noble **(IN PROGRESS)**
      - He may "ask" another noble `M` to talk; **(IN PROGRESS)**
      - He may wait until `T` seconds for any noble to answer; **(KINDA|REFACTOR)**
      - They talk for `X` seconds; **(KINDA|REFACTOR)**
      - Reactions:
        * If, at any time, `[KING SUMMONS]` one of the nobles, 1 `[NOBLE TALK TO KING]` and 1 abandons action and goes to next action;
        * If, while waiting for noble `M`, receives `[NOBLE TALK]` request from another noble, talk to another noble and abort first talk request;
    * Idle **(ALMOST DONE)**
      - Noble is idle for a certain duration;
      - Reactions:
        * `[KING SUMMONS]`, then `[NOBLE TALK TO KING]` **(DONE)**
        * `[KING END BALL]`, then `[NOBLE END BALL]` **(DONE)**
        * `[NOBLE DANCE]` request, then `[NOBLE DANCE]` using the request to build parameters **(TODO)**
    * Noble end ball **(DONE)**
      - Noble should leave the ball;


## Implementation

### TODO:

- [ ] Update noble alert to be a condition (there is a problem with multiple sem_posts now);
- [ ] Refactor action signal to work when calling from noble. Also refactor actions priority **(DOING)**;
- [ ] Organize stuff;
- [ ] Try substituting p2p to client-server lookalike model;
- [ ] Maybe make some event listener functionality?
  - Problem: performance probably goes to hell?
  - With add_listener({ .pthread_structure_type({ SEM, COND }), .events(linked list), .n_events(size_t), .pthread_structure(void*) })
  - remove_listener(listener*)
  - emit_event(event_type): unlocks all listeners accordingly

### Functionality/example:

There are functions called `king_spawn` and `noble_spawn` that spawn the king and the nobles present in the ball.

Each one receives a default actions sequence, that may change according to other nobles actions, to the kings requests, and to the environment.

Example of main:
```c
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
                  .duration=10
                }
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
     (king_action_t){
       .type=KING_TALK_TO_NOBLE,
       .params=&(king_talk_to_noble_params_t){
         .duration=3,
         .to_noble=-1 // dequeue noble. Otherwise, select random noble to summon
       }
     },
     (king_action_t){
       .type=KING_IDLE,
       .params=&(king_idle_params_t){
         .duration=2
       }
     },
     (king_action_t){
       .type=KING_TALK_TO_NOBLE,
       .params=&(king_talk_to_noble_params_t){
         .duration=10,
         .to_noble=0 // summons noble 0 to talk to him, even if he hadn't entered the queue
       }
     },
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
```

## Compiling

[TODO]: refactor entire project and use Makefile

```bash
gcc main3.c -lpthread
```

