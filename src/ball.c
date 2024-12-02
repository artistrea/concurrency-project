#include "king.h"
#include "noble.h"
#include "ball.h"
#include "events.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t n_nobles;

void (*global_render_fn)(char *king_text, size_t n_nobles, char *noble_texts[]) = NULL;

char *king_text;
char **noble_text;

void rerender() {
  global_render_fn(king_text, n_nobles, noble_text);
}

typedef struct {
  int *nobles_waiting;
  int queue_start_at;
  int size;
  pthread_mutex_t mutex;
} talk_to_king_queue_type;

talk_to_king_queue_type talk_to_king_queue = {
  .mutex=PTHREAD_MUTEX_INITIALIZER,
  .queue_start_at=0,
  .size=0
};

ball_is_over_info_type ball_is_over_info = {
  .mutex=PTHREAD_MUTEX_INITIALIZER,
  .is_over=0
};


talk_to_king_queue_info_type talk_to_king_queue_info = {
  .king_called_for=-1,
  .mutex=PTHREAD_MUTEX_INITIALIZER
};

void enter_talk_to_king_queue(int noble_id) {
  pthread_mutex_lock(&talk_to_king_queue.mutex);
    int i = (talk_to_king_queue.queue_start_at + talk_to_king_queue.size) % n_nobles;

    talk_to_king_queue.nobles_waiting[i] = noble_id;
    talk_to_king_queue.size++;
  pthread_mutex_unlock(&talk_to_king_queue.mutex);
}

void leave_talk_to_king_queue(int noble_id) {
  pthread_mutex_lock(&talk_to_king_queue.mutex);
    int went_through = 0, i = talk_to_king_queue.queue_start_at;

    while (went_through != talk_to_king_queue.size) {
      if (talk_to_king_queue.nobles_waiting[i] != -1) {
        went_through++;
      }
      if (talk_to_king_queue.nobles_waiting[i] == noble_id) {
        talk_to_king_queue.nobles_waiting[i] = -1;
        talk_to_king_queue.size--;
        break;
      }
      i = (i + 1) % n_nobles;
    }
  pthread_mutex_unlock(&talk_to_king_queue.mutex);

  pthread_mutex_lock(&talk_to_king_queue_info.mutex);
  if (talk_to_king_queue_info.king_called_for ==  noble_id) {
    talk_to_king_queue_info.king_called_for = -1;
  }
  pthread_mutex_unlock(&talk_to_king_queue_info.mutex);
}

int request_noble_to_talk_to_king(int noble_id) {
  int selected_noble_id;
  pthread_mutex_lock(&talk_to_king_queue.mutex);
    if (talk_to_king_queue.size) {
      while (talk_to_king_queue.nobles_waiting[talk_to_king_queue.queue_start_at] == -1) {
        if (talk_to_king_queue.queue_start_at >= n_nobles - 1) {
          talk_to_king_queue.queue_start_at = 0;
        } else {
          talk_to_king_queue.queue_start_at++;
        }
      }
    }

    if (noble_id == -1) {
      if (talk_to_king_queue.size) {
        selected_noble_id = talk_to_king_queue.nobles_waiting[talk_to_king_queue.queue_start_at];
        talk_to_king_queue.nobles_waiting[talk_to_king_queue.queue_start_at] = -1;
        talk_to_king_queue.size--;
        if (talk_to_king_queue.queue_start_at >= n_nobles - 1) {
          talk_to_king_queue.queue_start_at = 0;
        } else {
          talk_to_king_queue.queue_start_at++;
        }
      } else {
        selected_noble_id = lrand48() % n_nobles;
      }
    } else {
      selected_noble_id = noble_id;
    }
  pthread_mutex_unlock(&talk_to_king_queue.mutex);

  pthread_mutex_lock(&talk_to_king_queue_info.mutex);
    talk_to_king_queue_info.king_called_for = selected_noble_id;
  pthread_mutex_unlock(&talk_to_king_queue_info.mutex);
  return selected_noble_id;
}

nobles_trying_to_talk_to_type noble_trying_to_talk_to = {
  .mutex=PTHREAD_MUTEX_INITIALIZER,
  .arr=NULL
};

void create_ball_(
  void (*render_fn)(char *king_text, size_t n_nobles, char *noble_texts[]),
  king_action* first_king_action,
  noble_action_list* noble_actions_lists[]
) {
  events_manager_setup();

  talk_to_king_queue.nobles_waiting = realloc(talk_to_king_queue.nobles_waiting, n_nobles * sizeof(int));
  global_render_fn = render_fn;
  king_text = malloc(128 * sizeof(char));
  memset(king_text, 0, 128 * sizeof(char));

  while (noble_actions_lists[n_nobles] != NULL) {
    n_nobles++;
    printf("nobl: %lu\n", n_nobles);
  }

  noble_trying_to_talk_to.arr = malloc(sizeof(noble_wants_to_talk_to) * n_nobles);
  memset(noble_trying_to_talk_to.arr, -1, sizeof(noble_wants_to_talk_to) * n_nobles);

  noble_text = malloc(n_nobles * sizeof(char*));
  for (size_t i=0; i<n_nobles; i++) {
    noble_text[i] = malloc(128 * sizeof(char));
    memset(noble_text[i], 0, 128 * sizeof(char));
  }

  pthread_t king_thread, *noble_threads = malloc(sizeof(pthread_t) * n_nobles), events_thread;

  events_manager_thread_create(&events_thread);

  pthread_create(&king_thread, NULL, king_routine, (void*)first_king_action);

  for (size_t i = 0; i < n_nobles; i++) {
    noble_actions_lists[i]->id = i;
    pthread_create(&noble_threads[i], NULL, noble_routine, (void*)noble_actions_lists[i]);
  }
  rerender();

  pthread_join(king_thread, NULL);

  for (size_t i = 0; i < n_nobles; i++) {
    pthread_join(noble_threads[i], NULL);
  }

  // free(king_text);

  for (size_t i=0; i<n_nobles; i++) {
    // free(noble_text[i]);
  }
  // free(noble_text);
}


