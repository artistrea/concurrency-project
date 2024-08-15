# Noble Ball

The objective is to explore concurrency.

A noble ball will be implemented with the following specs:

[TODO]


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

