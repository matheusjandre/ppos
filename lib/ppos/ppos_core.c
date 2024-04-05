// GRR20215397 Matheus Henrique Jandre Ferraz

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

os_globals_t globals;

void ppos_init()
{
#ifdef DEBUG
  printf("(ppos_init) inicializando sistema.\n");
#endif

  setvbuf(stdout, 0, _IONBF, 0);

  getcontext(&globals.mainTask.context);
  globals.taskCounter = 0;
  globals.mainTask.id = 0;
  globals.currentTask = &(globals.mainTask);

  globals.readyQueue = NULL;

#ifdef DEBUG
  printf("(ppos_init) criando dispatcher.\n");
#endif
  task_init(&globals.dispatcherTask, dispatcher, NULL);

#ifdef DEBUG
  printf("(ppos_init) sistema inicializado.\n");
  printf("(ppos_init) mainTaskId: %d\n", globals.mainTask.id);
  printf("(ppos_init) currentTaskId: %d\n", globals.currentTask->id);
  printf("(ppos_init) taskCounter: %d\n", globals.taskCounter);
#endif
}

// retorna a próxima tarefa a ser executada, ou NULL se não houver nada para fazer
task_t *scheduler()
{
  task_t *nextTask = (task_t *)globals.readyQueue;
  globals.readyQueue = globals.readyQueue->next;
  return nextTask;
}

// dispatcher do sistema operacional
void dispatcher(void *arg)
{
#ifdef DEBUG
  printf("(dispatcher) iniciando dispatcher.\n");
#endif
  task_t *nextTask = NULL;

  // Remove a tarefa dispatcher da fila de prontas
  queue_remove((queue_t **)&globals.readyQueue, (queue_t *)&globals.dispatcherTask);
  // globals.taskCounter--;

  while (globals.taskCounter > 0)
  {
    nextTask = scheduler();

    if (nextTask)
    {
      queue_remove((queue_t **)&globals.readyQueue, (queue_t *)nextTask);

      task_switch(nextTask);

      switch (nextTask->status)
      {
      case READY:
        queue_append((queue_t **)&globals.readyQueue, (queue_t *)nextTask);
        break;
      case SUSPENDED:
        break;
      case TERMINATED:
        free(nextTask->context.uc_stack.ss_sp);
        nextTask->context.uc_stack.ss_sp = NULL;
        nextTask->context.uc_stack.ss_size = 0;
        nextTask = NULL;
        break;
      default:
        break;
      }
    }
  }

#ifdef DEBUG
  printf("(dispatcher) não há mais tarefas para executar.\n");
  printf("(dispatcher) encerrando dispatcher.\n");
#endif

  task_exit(0);
}

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
  // Inicializa o contexto da tarefa
  getcontext(&task->context);

  // Aloca a pilha para o contexto
  char *stack = malloc(STACKSIZE);
  if (stack)
  {
    task->context.uc_stack.ss_sp = stack;       // Stack pointer
    task->context.uc_stack.ss_size = STACKSIZE; // Tamanho da pilha
    task->context.uc_stack.ss_flags = 0;        // Zero indica que não há sinalização de uso da pilha
    task->context.uc_link = 0;                  // Contexto que será retomado ao término dessa tarefa
  }
  else
  {
    perror("(task_init) erro ao alocar pilha:");
    return -1;
  }

  // Define a função que a tarefa deve executar
  makecontext(&task->context, (void (*)(void))start_func, 1, arg);

  // Inicializa os outros campos da estrutura
  task->id = ++globals.taskCounter;
  task->status = READY;

  // Adiciona a tarefa na fila de prontas
  queue_append((queue_t **)&globals.readyQueue, (queue_t *)task);

#ifdef DEBUG
  printf("(task_init) tarefa %d inicializada.\n", task->id);
  printf("(task_init) taskCounter: %d\n", globals.taskCounter);
#endif

  return task->id;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
  // Checa se a tarefa existe
  if (!task)
  {
    perror("(task_switch) tarefa inexistente:");
    return -1;
  }

#ifdef DEBUG
  if (task_id() == globals.dispatcherTask.id)
  {
    printf("(task_switch) trocando de DISPATCHER para %d.\n", task->id);
  }
  else
  {
    printf("(task_switch) trocando de %d para DISPATCHER.\n", globals.currentTask->id);
  }

#endif

  // Salva a tarefa atual
  task_t *temp = globals.currentTask;

  // Atualiza a tarefa corrente
  globals.currentTask = task;
  globals.currentTask->status = RUNNING;

  // Troca de contexto
  swapcontext(&temp->context, &task->context);
  return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit(int exit_code)
{
  if (exit_code != 0)
  {
    printf("Tarefa %d terminada com erro: %d\n", globals.currentTask->id, exit_code);
  }

  --globals.taskCounter;
  globals.currentTask->status = TERMINATED;

  if (globals.currentTask->id == globals.mainTask.id)
  {
#ifdef DEBUG
    printf("(task_exit) mainTask finalizada.\n");
#endif

    task_switch(&globals.dispatcherTask);
    return;
  }

  if (globals.currentTask->id == globals.dispatcherTask.id)
  {

#ifdef DEBUG
    printf("(task_exit) dispatcherTask finalizada.\n");
    printf("(task_exit) encerrando sistema.\n");
#endif

    exit(exit_code);
  }

#ifdef DEBUG
  printf("(task_exit) tarefa %d finalizada.\n", globals.currentTask->id);
#endif

  task_switch(&globals.dispatcherTask);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
  // Checa se existe uma tarefa corrente
  if (!globals.currentTask)
  {
    perror("(task_id) tarefa inexistente:");
    return -1;
  }

  // #ifdef DEBUG
  //   printf("(task_id) tarefa corrente: %d.\n", globals.currentTask->id);
  // #endif

  // Retorna o id da tarefa corrente
  return globals.currentTask->id;
}

// a tarefa atual libera o processador para outra tarefa
void task_yield()
{
  if (!globals.currentTask)
  {
    perror("(task_yield) tarefa inexistente:");
    return;
  }

#ifdef DEBUG
  printf("(task_yield) tarefa %d liberou o processador.\n", globals.currentTask->id);
#endif
  // Atualiza o status da tarefa corrente
  globals.currentTask->status = READY;
  // Troca de contexto
  task_switch(&globals.dispatcherTask);
}