// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

os_globals_t ppos;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init()
{
#ifdef DEBUG
  printf("(ppos_init) inicializando sistema.\n");
#endif

  setvbuf(stdout, 0, _IONBF, 0);

  getcontext(&ppos.mainTask.context);
  ppos.taskCounter = 0;
  ppos.mainTask.id = 0;
  ppos.currentTask = &(ppos.mainTask);
  ppos.currentTask->staticPriority = PRIORITY_DEFAULT;
  ppos.currentTask->dynamicPriority = PRIORITY_DEFAULT;

  ppos.readyQueue = NULL;

#ifdef DEBUG
  printf("(ppos_init) criando dispatcher.\n");
#endif

  task_init(&ppos.dispatcherTask, dispatcher, NULL);

#ifdef DEBUG
  printf("(ppos_init) mainTaskId: %d\n", ppos.mainTask.id);
  printf("(ppos_init) currentTaskId: %d\n", ppos.currentTask->id);
  printf("(ppos_init) taskCounter: %d\n", ppos.taskCounter);
  printf("(ppos_init) sistema inicializado -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
#endif
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
  task->id = ++ppos.taskCounter;
  task->status = READY;
  task->staticPriority = PRIORITY_DEFAULT;
  task->dynamicPriority = PRIORITY_DEFAULT;

  // Adiciona a tarefa na fila de prontas
  queue_append((queue_t **)&ppos.readyQueue, (queue_t *)task);

#ifdef DEBUG
  printf("(task_init) tarefa %d inicializada.\n", task->id);
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
  if (task_id() == ppos.dispatcherTask.id)
  {
    printf("(task_switch) trocando de DISPATCHER para %d.\n", task->id);
  }
  else
  {
    printf("(task_switch) trocando de %d para DISPATCHER.\n", ppos.currentTask->id);
  }

#endif

  // Salva a tarefa atual
  task_t *temp = ppos.currentTask;

  // Atualiza a tarefa corrente
  ppos.currentTask = task;
  ppos.currentTask->status = RUNNING;

  // // Remove a tarefa atual da fila de prontas
  // queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)task);

  // Troca de contexto
  swapcontext(&temp->context, &task->context);
  return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit(int exit_code)
{
  if (exit_code != 0)
  {
    printf("Tarefa %d terminada com erro: %d\n", ppos.currentTask->id, exit_code);
  }

  --ppos.taskCounter;
  ppos.currentTask->status = TERMINATED;

  if (ppos.currentTask->id == ppos.dispatcherTask.id)
  {

#ifdef DEBUG
    printf("(task_exit) dispatcherTask finalizada.\n");
    printf("(task_exit) encerrando sistema.\n");
#endif

    exit(exit_code);
  }

#ifdef DEBUG
  printf("(task_exit) tarefa %d finalizada.\n", ppos.currentTask->id);
#endif

  task_switch(&ppos.dispatcherTask);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
  // Checa se existe uma tarefa corrente
  if (!ppos.currentTask)
  {
    perror("(task_id) tarefa inexistente:");
    return -1;
  }

  // #ifdef DEBUG
  //   printf("(task_id) tarefa corrente: %d.\n", ppos.currentTask->id);
  // #endif

  // Retorna o id da tarefa corrente
  return ppos.currentTask->id;
}

// a tarefa atual libera o processador para outra tarefa
void task_yield()
{
  if (!ppos.currentTask)
  {
    perror("(task_yield) tarefa inexistente:");
    return;
  }

#ifdef DEBUG
  printf("(task_yield) tarefa %d liberou o processador.\n", ppos.currentTask->id);
#endif
  // Atualiza o status da tarefa corrente
  ppos.currentTask->status = READY;
  // Troca de contexto
  task_switch(&ppos.dispatcherTask);
}

// dispatcher do sistema operacional
void dispatcher(void *arg)
{
#ifdef DEBUG
  printf("(dispatcher) iniciando dispatcher.\n");
#endif
  task_t *nextTask = NULL;

  // Remove a tarefa dispatcher da fila de prontas
  queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)&ppos.dispatcherTask);

  while (ppos.taskCounter > 0)
  {
    nextTask = scheduler();

    if (nextTask)
    {
      // Remove a tarefa atual da fila de prontas
      queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)nextTask);

      // Troca de contexto
      task_switch(nextTask);

      // Trata o status da tarefa após a execução
      switch (nextTask->status)
      {
      case READY:
        queue_append((queue_t **)&ppos.readyQueue, (queue_t *)nextTask);
        break;
      case TERMINATED:
        // Libera a pilha da tarefa
        free(nextTask->context.uc_stack.ss_sp);
        nextTask->context.uc_stack.ss_sp = NULL;
        nextTask->context.uc_stack.ss_size = 0;
        nextTask = NULL;
        break;
      default:
#ifdef DEBUG
        printf("(dispatcher) status da tarefa %d: %d\n", nextTask->id, nextTask->status);
#endif
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

// retorna a próxima tarefa a ser executada, ou NULL se não houver nada para fazer
task_t *scheduler()
{
  task_t *selectedTask = NULL, *tempTask = (task_t *)ppos.readyQueue;
  int tempTaskPriority = 0, selectedTaskPriority = 0;

  // Seleciona a tarefa com maior prioridade
  if (tempTask)
  {
    selectedTask = tempTask;
    tempTask = tempTask->next;

    // Percorre a fila de tarefas prontas
    while (tempTask != (task_t *)ppos.readyQueue)
    {
      tempTaskPriority = task_getprio(tempTask) + tempTask->dynamicPriority;             // Prioridade dinâmica da tarefa temporária
      selectedTaskPriority = task_getprio(selectedTask) + selectedTask->dynamicPriority; // Prioridade dinâmica da tarefa selecionada

      if (tempTaskPriority < selectedTaskPriority) // Se a prioridade da tarefa temporária for menor que a da tarefa selecionada
      {
        task_to_age(selectedTask); // Envelhece a tarefa selecionada
        selectedTask = tempTask;   // Seleciona a tarefa temporária
      }
      else // Se a prioridade da tarefa temporária for maior ou igual a da tarefa selecionada
      {
        task_to_age(tempTask); // Envelhece a tarefa temporária
      }

      tempTask = tempTask->next;
    }

    selectedTask->dynamicPriority = PRIORITY_DEFAULT; // Reseta a prioridade dinâmica da tarefa selecionada
  }

#ifdef DEBUG
  if (selectedTask)
  {
    printf("(scheduler) próxima tarefa a ser executada: %d.\n", selectedTask->id);
  }
  else
  {
    printf("(scheduler) não há tarefas para executar.\n");
  }
#endif

  return selectedTask; // Retorna a tarefa selecionada
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio(task_t *task, int prio)
{
  if (!task && !ppos.currentTask)
  {
    perror("(task_setprio) tarefa inexistente:");
    return;
  }

  if (prio < PRIORITY_MIN || prio > PRIORITY_MAX)
  {
    perror("(task_setprio) prioridade inválida:");
    return;
  }

#ifdef DEBUG
  int id = task ? task->id : ppos.currentTask->id;
  printf("(task_setprio) prioridade da tarefa %d alterada para %d.\n", id, prio);
#endif

  if (!task)
  {
    ppos.currentTask->staticPriority = prio;
  }
  else
  {
    task->staticPriority = prio;
  }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
  if (!task && !ppos.currentTask)
  {
    perror("(task_getprio) tarefa inexistente:");
    return -1;
  }

  return task ? task->staticPriority : ppos.currentTask->staticPriority;
}

// envelhece a prioridade dinâmica de uma tarefa
int task_to_age(task_t *task)
{
  if (!task)
  {
    perror("(task_to_age) tarefa inexistente:");
    return -1;
  }

  if (task->dynamicPriority > PRIORITY_MIN && task->dynamicPriority < PRIORITY_MAX)
    task->dynamicPriority += PRIORITY_AGING;

  return 0;
}