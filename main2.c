// os nobres chegam e vão cumprimentar o rei, então ficam conversando
// quando todos os nobres chegarem, eles podem dnçar na pista
// para dançar na pista, um nobre deve encontrar um par para tal
// para dançar na pista, a pista não pode estar lotada
  // caso se queira dançar na pista e ela esteja lotada, espera-se até T segundos para ver se ela libera
  // caso se esteja esperando a pista liberar e alguém conversa com algum dos nobres esperando, o tempo de espera T reseta
// caso o rei que esteja fazendo um anúncio, todos os nobres devem parar para ouvir
// ao longo da festa, os nobres podem ser convocados para conversar com o rei,
  // tendo que ir imediatamente ao seu encontro, e furando a fila de espera, caso exista

// o rei faz anúncios
// o rei conversa com as pessoas que estão na fila,
// o rei pode convocar algum nobre em específico para vir até ele

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#define N_NOBLES 3

int noble_ids[N_NOBLES];

typedef enum {
  TALK_TO_KING, TALK_TO_NOBLE, DANCE_WITH_NOBLE
} noble_action_type_t;

typedef int talk_to_noble_params_t;
typedef int dance_with_noble_params_t;

typedef struct noble_action {
  noble_action_type_t action;
  int priority;
  void *action_params;
  struct noble_action *next_noble_action;
} noble_action_t;

typedef struct {
  noble_action_t *head;
  int size;
} noble_actions_list_t;

pthread_mutex_t talk_to_king_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t talk_to_king_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int next_on_king_queue = -1;

typedef enum {
  LISTEN_TO_NOBLE_ON_QUEUE, REQUIRE_NOBLE, END_BALL
} king_action_type_t;

typedef struct king_action {
  king_action_type_t action;
  void *action_params;
  struct king_action *next_king_action;
} king_action_t;

typedef struct {
  king_action_t *head;
  int size;
} king_actions_list_t;

sem_t talk_to_king_semaphore;

// TODO: é necessário trocar todos os sleep por sem_timedwait para permitir que o rei interrompa qualquer sleep

void * nobles_fn(void *idv) {
  int *id = (int*)idv;
  noble_actions_list_t actions_list;
  actions_list.size = 1;
  actions_list.head = malloc(sizeof(noble_action_t));
  actions_list.head->next_noble_action = NULL;
  actions_list.head->action = TALK_TO_KING;
  actions_list.head->priority = 1;

  noble_action_t *current_action;
  while (1) {
    current_action = actions_list.head;
    if (current_action == NULL) {
      sleep(1);
      continue;
    }
    actions_list.head = current_action->next_noble_action;
    actions_list.size--;

    switch (current_action->action) {
      case TALK_TO_KING: {
        // TODO: utilizar variável condição, para permitir que o rei acorde todos caso necessário
        pthread_mutex_lock(&talk_to_king_mutex); // mutex only for nobles so that only one noble may write to next on queue
        pthread_mutex_lock(&talk_to_king_queue_mutex); // shared mutex to let king read safely
        next_on_king_queue = *id;
        pthread_mutex_unlock(&talk_to_king_queue_mutex);
        sem_wait(&talk_to_king_semaphore);
        pthread_mutex_unlock(&talk_to_king_mutex);

        // falando com o rei
        sleep(1);

        break;
      }
      case TALK_TO_NOBLE: {
        talk_to_noble_params_t* other_noble_id = current_action->action_params;
        break;
      }
      case DANCE_WITH_NOBLE: {
        dance_with_noble_params_t* other_noble_id = current_action->action_params;
        break;
      }
    }

    // adiciona ação à lista caso não exista próxima

    // ao adicionar a ação de interagir com outro nobre, inserir a ação na lista de ações do outro nobre também
    // também deve avisar o outro nobre de alguma maneira (deve resetar o tempo de espera pela pista de dança)
    sleep(1);

    printf("opa meu nobre %d\n", *id);
  }

  pthread_exit(0);
};

void * king_fn() {
  while (1) {
    sleep(2);
    printf("long live the me\n");
  }

  pthread_exit(0);
}


int main() {
  pthread_t king_thread, announcer_thread, noble_threads[N_NOBLES];

  sem_init(&talk_to_king_semaphore, 0, 0);

  size_t i;
  for (i = 0; i < N_NOBLES; i++) {
    noble_ids[i] = i;
    pthread_create(&noble_threads[i], NULL, nobles_fn, (void*) &noble_ids[i]);
  }

  pthread_create(&king_thread, NULL, king_fn, (void*)1);


  for (i = 0; i < N_NOBLES; i++) 
    pthread_join(noble_threads[i], NULL);

  pthread_join(king_thread, NULL);

  return 0;
}



