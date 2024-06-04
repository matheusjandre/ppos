// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdlib.h>
#include <string.h>
#include "ppos.h"
#include "ppos_data.h"

// Busy waiting para entrar na seção crítica
void enter_critical_session(char *lock)
{
  while (__sync_fetch_and_or(lock, 1))
    ;
}

// Sai da seção crítica
void leave_critical_session(char *lock)
{
  *lock = 0;
}

// inicializa um semáforo com valor inicial "value"
int sem_init(semaphore_t *s, int value)
{
  if (!s)
    return -1;

#if defined(DEBUG_SEM_INIT) || defined(DEBUG_ALL)
  debug_print("(sem_init) inicializando o semáforo com valor %d\n", value);
#endif

  s->dead = 0;
  s->counter = value;
  s->waitingQueue = NULL;

  return 0;
}

// requisita o semáforo
int sem_down(semaphore_t *s)
{
  if (s->dead)
    return -1;

  enter_critical_session(&s->ocupied);
  s->counter--; // decrementa o contador
  leave_critical_session(&s->ocupied);

  if (s->counter < 0)
    task_suspend((task_t **)&s->waitingQueue); // suspende a tarefa se o contador for negativo

#if defined(DEBUG_SEM_DOWN) || defined(DEBUG_ALL)
  debug_print("(sem_down) semáforo requisitado, contador: %d\n", s->counter);
#endif
  return 0;
}

// libera o semáforo
int sem_up(semaphore_t *s)
{
  if (s->dead)
    return -1;

  enter_critical_session(&s->ocupied);
  s->counter++; // incrementa o contador
  leave_critical_session(&s->ocupied);

  if (s->counter <= 0)
    task_awake((task_t *)s->waitingQueue, (task_t **)&s->waitingQueue); // acorda a primeira tarefa se o contador for menor ou igual a zero

#if defined(DEBUG_SEM_UP) || defined(DEBUG_ALL)
  debug_print("(sem_up) semáforo liberado, contador: %d\n", s->counter);
#endif
  return 0;
}

// "destroi" o semáforo, liberando as tarefas bloqueadas
int sem_destroy(semaphore_t *s)
{
  if (s->dead)
    return -1;

  while (s->waitingQueue)
    task_awake((task_t *)s->waitingQueue, (task_t **)&s->waitingQueue); // acorda todas as tarefas bloqueadas

  s->dead = 1;    // marca o semáforo como destruído
  s->counter = 0; // zera o contador

#if defined(DEBUG_SEM_DESTROY) || defined(DEBUG_ALL)
  debug_print("(sem_destroy) semáforo destruído\n");
#endif

  return 0;
}

// inicializa uma fila para até max mensagens de size bytes cada
int mqueue_init(mqueue_t *queue, int max, int size)
{
  if (!queue) // Se a fila não existir retorna -1
    return -1;

  if (max <= 0) // Se o número máximo de mensagens for menor ou igual a zero retorna -2
    return -2;

  if (size <= 0) // Se o tamanho da mensagem for menor ou igual a zero retorna -3
    return -3;

#if defined(DEBUG_QUEUE_INIT) || defined(DEBUG_ALL)
  debug_print("(mqueue_init) inicializando a fila com %d mensagens de %d bytes\n", max, size);
#endif

  queue->dead = 0;           // Inicializa a flag de destruição
  queue->maxMessages = max;  // Inicializa o número máximo de mensagens
  queue->messageSize = size; // Inicializa o tamanho de cada mensagem
  queue->buffer = NULL;      // Inicializa o buffer

  sem_init(&queue->s_buffer, 1);    // Inicializa o semáforo do buffer
  sem_init(&queue->s_vacancy, max); // Inicializa o semáforo de vagas
  sem_init(&queue->s_item, 0);      // Inicializa o semáforo de itens

#if defined(DEBUG_QUEUE_INIT) || defined(DEBUG_ALL)
  debug_print("(mqueue_init) semáforos inicializados\n");
#endif

  return 0;
}

// envia uma mensagem para a fila
int mqueue_send(mqueue_t *queue, void *msg)
{
#if defined(DEBUG_QUEUE_RECV) || defined(DEBUG_ALL)
  debug_print("(mqueue_recv) tentando enviar uma mensagem para a fila.\n");
#endif

  if (queue->dead) // Se a fila ou a mensagem não existirem retorna -1
    return -1;

  if (!msg) // Se a mensagem não existir retorna -2
    return -2;

  if (sem_down(&queue->s_vacancy) < 0) // Decrementa o semáforo de vagas
    return -3;
  if (sem_down(&queue->s_buffer) < 0) // Decrementa o semáforo do buffer
    return -4;

  message_t *message = (message_t *)malloc(sizeof(message_t)); // Aloca espaço para a nova mensagem
  if (!message)                                                // Se não conseguir alocar espaço retorna -3
    return -5;

  message->next = NULL;                          // Inicializa a nova mensagem
  message->prev = NULL;                          // Inicializa a nova mensagem
  message->content = malloc(queue->messageSize); // Aloca espaço para a mensagem
  if (!message->content)                         // Se não conseguir alocar espaço retorna -4
    return -5;

  memcpy(message->content, msg, queue->messageSize); // Copia a mensagem para a nova mensagem

  if (queue_append((queue_t **)&queue->buffer, (queue_t *)message) < 0) // Adiciona a nova mensagem ao buffer
    return -6;

  if (sem_up(&queue->s_buffer) < 0) // Incrementa o semáforo do buffer
    return -7;
  if (sem_up(&queue->s_item) < 0) // Incrementa o semáforo de itens
    return -8;

#if defined(DEBUG_QUEUE_SEND) || defined(DEBUG_ALL)
  debug_print("(mqueue_send) mensagem enviada a fila\n");
#endif

  return 0;
}

// recebe uma mensagem da fila
int mqueue_recv(mqueue_t *queue, void *msg)
{
#if defined(DEBUG_QUEUE_RECV) || defined(DEBUG_ALL)
  debug_print("(mqueue_recv) tentando receber mensagem da fila.\n");
#endif

  if (queue->dead) // Se a fila não existir retorna -1
    return -1;

  if (!msg) // Se a mensagem não existir retorna -2
    return -2;

  if (sem_down(&queue->s_item) < 0) // Decrementa o semáforo de itens
    return -3;

  if (sem_down(&queue->s_buffer) < 0) // Decrementa o semáforo do buffer
    return -4;

  message_t *message = (message_t *)queue->buffer;                      // Pega a mensagem do buffer
  if (queue_remove((queue_t **)&queue->buffer, (queue_t *)message) < 0) // Remove a mensagem do buffer
    return -5;

  if (sem_up(&queue->s_buffer) < 0) // Incrementa o semáforo do buffer
    return -6;
  if (sem_up(&queue->s_vacancy) < 0) // Incrementa o semáforo de vagas
    return -7;

  memcpy(msg, message->content, queue->messageSize); // Copia a mensagem do buffer
  free(message->content);                            // Libera o espaço da mensagem
  free(message);                                     // Libera o espaço da nova mensagem

#if defined(DEBUG_QUEUE_RECV) || defined(DEBUG_ALL)
  debug_print("(mqueue_send) recebendo mensagem da fila\n");
#endif

  return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy(mqueue_t *queue)
{
  if (queue->dead) // Se a fila ou a mensagem não existirem retorna -1
    return -1;

  queue->dead = 1;
  queue->maxMessages = 0;
  queue->messageSize = 0;

  while (queue->buffer)
  {
    message_t *message = (message_t *)queue->buffer;
    if (queue_remove((queue_t **)&queue->buffer, (queue_t *)message) < 0) // Remove a mensagem do buffer
      return -2;

    free(message->content); // Libera o espaço da mensagem
    free(message);          // Libera o espaço da nova mensagem
  }

  if (sem_destroy(&queue->s_buffer) < 0)
    return -3;
  if (sem_destroy(&queue->s_vacancy) < 0)
    return -4;
  if (sem_destroy(&queue->s_item) < 0)
    return -5;

  return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs(mqueue_t *queue)
{
  if (!queue)
    return -1;

  return queue_size((queue_t *)queue->buffer);
}
