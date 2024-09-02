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
  KING_EMITTED_NEXT_ON_QUEUE = 0b00001,
  KING_EMITTED_BALL_IS_OVER =  0b00010,
  NOBLE_EMITTED_PRESENTED_HIMSELF_TO_KING =  0b00100,
  KING_EMITTED_DISMISS_NOBLE_TALKING = 0b01000,
  NOBLE_EMITTED_TALK_TO_NOBLE =  0b10000,
} ball_event_type;

typedef struct {
  size_t from_noble;
  size_t to_noble;
} noble_emitted_talk_to_noble_params;

typedef struct {
  int to_noble;
  int duration;
  int noble_has_accepted; // -1 means it hasn't
} noble_wants_to_talk_to;

typedef struct {
  pthread_mutex_t mutex;
  noble_wants_to_talk_to *arr; // size of n_nobles
} nobles_trying_to_talk_to_type;

extern nobles_trying_to_talk_to_type noble_trying_to_talk_to;

extern size_t n_nobles;

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

