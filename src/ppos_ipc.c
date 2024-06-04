// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdlib.h>
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
