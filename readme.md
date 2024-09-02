https://github.com/artistrea/concurrency-project

# Noble Ball

The objective is to explore concurrency.

A noble ball will be implemented with the following specs:

1. The nobles and king arrive with a predetermined list of actions to do;

2. The king has the following actions/states:
    * Talk to a noble
      - He may summon any noble `M`;
      - If he doesn't specify a noble (`M=-1`), he chooses the next noble waiting on queue;
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
        * If waiting on the queue and `[KING END BALL]`, then `[NOBLE END BALL]`
        * If waiting on the queue and `[KING SUMMONS]` him occurs or already occurred, then present himself to king;
    * Talk to another noble
      - He may "ask" another noble `M` to talk;
      - He may wait until `T` seconds for noble `M` to answer;
      - They talk for `X` seconds;
      - Reactions:
        * `[KING END BALL]`, then `[NOBLE END BALL]`
        * If, at any time, `[KING SUMMONS]` one of the nobles, 1 `[NOBLE TALK TO KING]` and 1 abandons action and goes to next action;
    * Idle
      - Noble is idle for a certain duration;
      - Reactions:
        * `[KING SUMMONS]`, then `[NOBLE TALK TO KING]`
        * `[KING END BALL]`, then `[NOBLE END BALL]`
        * `[NOBLE TALK TO]` request, then `[NOBLE TALK TO]` using the request to build parameters
    * Noble end ball
      - Noble should leave the ball;


## Implementation

### Functionality/example:

There are functions called `king_spawn` and `noble_spawn` that spawn the king and the nobles present in the ball.

Each one receives a default actions sequence, that may change according to other nobles actions, to the kings requests, and to the environment.

Look at `src/main.c` to see more.

## Compiling and Running

To build the executable to ./bin

```bash
make
```

To build debug version:

```bash
# WARNING: you may need to run `make clean` if the *.o files were already built without debug
make debug
```

Then you can run the executable through:

```bash
./bin
```

Or, if you built for debug:

```bash
gdb ./bin
```

