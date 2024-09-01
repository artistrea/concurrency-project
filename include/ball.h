// this is supposed to manage all of the environmental part of the ball
// since the ball creation to the events that may happen or be emitted

#ifndef BALL_H
#define BALL_H

#include "king.h"
#include "noble.h"
#include "events.h"
#include <string.h>
#include <stdio.h>

void rerender();

extern char* king_text;
extern char** noble_text;

#define ball_set_noble_text(noble_id, ...) \
  do {\
    sprintf(noble_text[noble_id], __VA_ARGS__);\
    rerender();\
    \
  } while(0);

#define ball_set_king_text(...) \
  do {\
    sprintf(king_text, __VA_ARGS__);\
    rerender();\
    \
  } while(0);

void create_ball_(
  void (*render_fn)(char *king_text, size_t n_nobles, char *noble_texts[]),
  king_action* first_king_action,
  noble_action_list* noble_actions_lists[]
);

// should only be called once per 
#define create_ball(render_fn, first_king_action, ...)\
  create_ball_(render_fn, first_king_action, (noble_action_list*[]){ __VA_ARGS__, NULL })

typedef enum {
  KING_EMITTED_NEXT_ON_QUEUE = 0b0001,
  KING_EMITTED_BALL_IS_OVER =  0b0010,
  NOBLE_EMITTED_PRESENTED_HIMSELF_TO_KING =  0b0100,
  KING_EMITTED_DISMISS_NOBLE_TALKING = 0b1000
  /* KING_EMITTED_BALL_IS_OVER =  0b0010, */
} ball_event_type;


typedef struct {
  int king_called_for;
  pthread_mutex_t mutex;
} talk_to_king_queue_info_type;

typedef struct {
  int is_over;
  pthread_mutex_t mutex;
} ball_is_over_info_type;

// TODO: implement writer/reader
extern ball_is_over_info_type ball_is_over_info;

extern talk_to_king_queue_info_type talk_to_king_queue_info;

void enter_talk_to_king_queue(int noble_id);

int request_noble_to_talk_to_king(int noble_id);

void leave_talk_to_king_queue(int noble_id);

#endif // BALL_H

