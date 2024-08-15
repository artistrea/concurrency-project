# Noble Ball

The objective is to explore concurrency.

A noble ball will be implemented with the following specs:

1. The nobles and king arrive with a predetermined list of actions to do;

2. The king has the following actions/states:
  * Talk to a noble
    - He may summon any noble;
    - If he doesn't specify a noble (value=-1), he chooses the next noble waiting on queue;
    - If he doesn't specify a noble and there are no nobles on queue, he summons a random noble;
    - The nobles have at most 1 second to present themselves;
  * Idle
    - King is simply idle;
  * End ball
    - Should end program;

3. A noble has the following actions/states and reactions:
  * Talk to the king
    - He enters a queue to talk to the king;
    - If king selected him to talk to, he presents himself to the king;
    - Waits for king dismissal;
    - Reactions:
      * If waiting on the queue and [KING END BALL], then [NOBLE END BALL]
      * If waiting on the queue and [KING SUMMONS] him occurs or already occurred, then present himself to king;
  * Talk
    - He may "ask" another noble M to talk;
    - He may wait until T seconds for any noble to answer;
    - They talk for X seconds;
    - Reactions:
      * If, at any time, [KING SUMMONS] one of the nobles, 1 [NOBLE TALK TO KING] and 1 abandons action and goes to next action;
      * If, while waiting for noble M, receives [NOBLE TALK] request from another noble, talk to another noble and abort first talk request;
  * Dance
    - He may "ask" another noble M to dance;
    - He may wait until T seconds for any noble to answer;
    - If noble answers, wait for up to T seconds for dance floor to have enough space for another pair;
    - If has enough space in T seconds, they go and dance for X seconds;
    - Reactions:
      * If, at any time, [KING SUMMONS] one of the nobles, he must goto [NOBLE TALK TO KING].
        1. If this happens while they are waiting for the dance floor, the one left may wait for T more seconds before abandoning action;
      * [KING END BALL], then [NOBLE END BALL]
      * If, while waiting for noble M, receives [NOBLE DANCE] request from another noble, dance with another noble and abort first dance request;
      * If, while waiting for noble M or while waiting for dance floor to be available, receives [NOBLE TALK] request
        1. He will talk to the noble who requested it, and after finishing the pair will wait for up to T more seconds for the dance floor;
  * Idle
    - Noble is idle for a certain duration;
    - Reactions:
      * [KING SUMMONS], then [NOBLE TALK TO KING]
      * [KING END BALL], then [NOBLE END BALL]
      * [NOBLE DANCE] request, then [NOBLE DANCE] using the request to build parameters
  * Noble end ball
    - Noble should leave the ball;


## Implementation

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

