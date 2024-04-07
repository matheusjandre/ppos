// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"

ppos_environment_t ppos;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init()
{
#ifdef DEBUG
  printf("(ppos_init) inicializando sistema.\n");
#endif

  setvbuf(stdout, 0, _IONBF, 0);

  // Prepara o ambiente do ppos
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

  task_init(&ppos.dispatcherTask, dispatcher, NULL); // Cria a tarefa dispatcher
  ppos.dispatcherTask.type = SYSTEM_TASK;            // Atribui o tipo da tarefa dispatcher como tarea do sistema

#ifdef DEBUG
  printf("(ppos_init) inicializando tratador de sinais.\n");
#endif

  // Inicializa o tratador de sinais do clock
  ppos.signalHandler.sa_handler = tick_handler; // Função de tratamento de sinais
  sigemptyset(&ppos.signalHandler.sa_mask);     // Máscara de sinais a serem bloqueados durante a execução do sinal
  ppos.signalHandler.sa_flags = 0;              // Opções de chamadas de sistema

  if (sigaction(SIGALRM, &ppos.signalHandler, 0) < 0)
  {
    perror("(ppos_init) erro ao instalar tratador de sinais:");
    exit(1);
  }

#ifdef DEBUG
  printf("(ppos_init) inicializando temporizador.\n");
#endif

  // Inicializa o temporizador
  ppos.clock.it_value.tv_usec = CLOCK_FIRST_SHOT_U;  // Primeiro disparo, em micro-segundos
  ppos.clock.it_value.tv_sec = CLOCK_FIRST_SHOT_S;   // Primeiro disparo, em segundos
  ppos.clock.it_interval.tv_usec = CLOCK_INTERVAL_U; // Disparos subsequentes, em micro-segundos
  ppos.clock.it_interval.tv_sec = CLOCK_INTERVAL_S;  // Disparos subsequentes, em segundos

  if (setitimer(ITIMER_REAL, &ppos.clock, 0) < 0)
  {
    perror("(ppos_init) erro ao iniciar temporizador:");
    exit(1);
  }

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
  getcontext(&task->context); // Inicializa o contexto da tarefa

  char *stack = malloc(STACKSIZE); // Aloca a pilha para o contexto
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

  makecontext(&task->context, (void (*)(void))start_func, 1, arg); // Define a função que a tarefa deve executar

  task->id = ++ppos.taskCounter;                               // ID da tarefa
  task->staticPriority = PRIORITY_DEFAULT;                     // Prioridade estática da tarefa
  task->dynamicPriority = PRIORITY_DEFAULT;                    // Prioridade dinâmica da tarefa
  task->type = USER_TASK;                                      // Tipo da tarefa
  task->status = READY;                                        // Status da tarefa
  queue_append((queue_t **)&ppos.readyQueue, (queue_t *)task); // Adiciona a tarefa na fila de prontas

#ifdef DEBUG
  printf("(task_init) tarefa %d inicializada.\n", task->id);
#endif

  return task->id;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
  if (!task) // Se a tarefa não existir
  {
    perror("(task_switch) tarefa inexistente:");
    return -1;
  }

#ifdef DEBUG
  if (task_id() == ppos.dispatcherTask.id)
    printf("(task_switch) trocando de DISPATCHER para %d.\n", task->id);
  else
    printf("(task_switch) trocando de %d para DISPATCHER.\n", ppos.currentTask->id);
#endif
  task_t *temp = ppos.currentTask; // Salva a tarefa atual

  ppos.currentTask = task;                           // Atualiza a tarefa corrente
  ppos.currentTask->status = RUNNING;                // Atualiza o status da tarefa corrente
  ppos.currentTask->quantumCounter = QUANTA_DEFAULT; // Reinicia o contador de quantum
  swapcontext(&temp->context, &task->context);       // Troca de contexto

  return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit(int exit_code)
{
  if (exit_code != 0)
    printf("Tarefa %d terminada com erro: %d\n", ppos.currentTask->id, exit_code);

  ppos.taskCounter--;                    // Decrementa o contador de tarefas
  ppos.currentTask->status = TERMINATED; // Atualiza o status da tarefa corrente

  if (ppos.currentTask->id == ppos.dispatcherTask.id)
  {
#ifdef DEBUG
    printf("(task_exit) encerrando sistema.\n");
#endif
    exit(exit_code); // Encerra o sistema
  }

#ifdef DEBUG
  printf("(task_exit) tarefa %d finalizada.\n", ppos.currentTask->id);
#endif

  task_switch(&ppos.dispatcherTask); // Volta para o dispatcher
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
  if (!ppos.currentTask) // Checa se existe uma tarefa corrente
  {
    perror("(task_id) tarefa inexistente:");
    return -1;
  }

  return ppos.currentTask->id; // Retorna o id da tarefa corrente
}

// a tarefa atual libera o processador para outra tarefa
void task_yield()
{
  if (!ppos.currentTask) // Checa se existe uma tarefa corrente
  {
    perror("(task_yield) tarefa inexistente:");
    return;
  }

#ifdef DEBUG
  printf("(task_yield) tarefa %d liberou o processador.\n", ppos.currentTask->id);
#endif

  ppos.currentTask->status = READY;  // Atualiza o status da tarefa corrente
  task_switch(&ppos.dispatcherTask); // Troca de contexto
}

// trata os sinais do temporizador do sistema
void tick_handler(int signum)
{
#ifdef DEBUG
  printf("(tick_handler) tarefa atual: %d - %d\n", ppos.currentTask->id, ppos.currentTask->quantumCounter);
#endif

  // Checa se a tarefa corrente é do tipo sistema ou se o contador de quantum é maior que zero
  if (ppos.currentTask->type == SYSTEM_TASK || --ppos.currentTask->quantumCounter > 0)
    return;

  task_yield();
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
      queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)nextTask); // Remove a tarefa atual da fila de prontas
      task_switch(nextTask);                                           // Troca de contexto

      // Trata o status da tarefa após a execução
      switch (nextTask->status)
      {
      case READY:
        queue_append((queue_t **)&ppos.readyQueue, (queue_t *)nextTask); // Adiciona a tarefa na fila de prontas
        break;
      case TERMINATED:
        free(nextTask->context.uc_stack.ss_sp);  // Libera a pilha da tarefa
        nextTask->context.uc_stack.ss_sp = NULL; // Reseta o ponteiro da pilha
        nextTask->context.uc_stack.ss_size = 0;  // Reseta o tamanho da pilha
        nextTask = NULL;                         // Reseta o ponteiro da tarefa
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

  if (tempTask) // Se a fila de tarefas prontas não estiver vazia
  {
    selectedTask = tempTask;
    tempTask = tempTask->next;

    while (tempTask != (task_t *)ppos.readyQueue) // Percorre a fila de tarefas prontas
    {
      tempTaskPriority = task_getprio(tempTask) + tempTask->dynamicPriority;             // Prioridade dinâmica da tarefa temporária
      selectedTaskPriority = task_getprio(selectedTask) + selectedTask->dynamicPriority; // Prioridade dinâmica da tarefa selecionada

      if (tempTaskPriority < selectedTaskPriority) // Se a prioridade da tarefa temporária for menor que a da tarefa selecionada
      {
        task_to_age(selectedTask); // Envelhece a tarefa selecionada
        selectedTask = tempTask;   // Seleciona a tarefa temporária
      }
      else                     // Se a prioridade da tarefa temporária for maior ou igual a da tarefa selecionada
        task_to_age(tempTask); // Envelhece a tarefa temporária

      tempTask = tempTask->next; // Avança para a próxima tarefa
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

  if (task->id == ppos.dispatcherTask.id)
  {
    perror("(task_setprio) dispatcher não pode ter prioridade alterada:");
    return;
  }

#ifdef DEBUG
  int id = task ? task->id : ppos.currentTask->id;
  printf("(task_setprio) prioridade da tarefa %d alterada para %d.\n", id, prio);
#endif

  if (task)
    task->staticPriority = prio; // Atribui a prioridade estática da tarefa passada
  else
    ppos.currentTask->staticPriority = prio; // Atribui a prioridade estática da tarefa corrente
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
  if (!task && !ppos.currentTask) // Se a tarefa não existir e a tarefa corrente não existir
  {
    perror("(task_getprio) tarefa inexistente:");
    return -1;
  }

  return task ? task->staticPriority : ppos.currentTask->staticPriority; // Retorna a prioridade estática da tarefa passada ou da tarefa corrente
}

// envelhece a prioridade dinâmica de uma tarefa
int task_to_age(task_t *task)
{
  if (!task) // Se a tarefa não existir
  {
    perror("(task_to_age) tarefa inexistente:");
    return -1;
  }

  if (task->dynamicPriority > PRIORITY_MIN && task->dynamicPriority < PRIORITY_MAX) // Se a prioridade dinâmica da tarefa estiver dentro dos limites
    task->dynamicPriority += PRIORITY_AGING;                                        // Envelhece a prioridade dinâmica da tarefa

  return 0;
}