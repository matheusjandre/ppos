// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"

ppos_environment_t ppos; // Variáveis globais do sistema operacional

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init()
{
#if defined(DEBUG_PPOS_INIT) || defined(DEBUG_ALL)
  debug_print("(ppos_init) inicializando sistema.\n");
#endif
  setvbuf(stdout, 0, _IONBF, 0); // Desativa o buffer da saída padrão (stdout), usado pela função printf

  getcontext(&ppos.mainTask.context);               // Salva o contexto da tarefa main
  ppos.mainTask.id = 0;                             // ID da tarefa main
  ppos.mainTask.activations = 1;                    // Contador de ativações da tarefa main
  ppos.mainTask.staticPriority = PRIORITY_DEFAULT;  // Prioridade estática da tarefa main
  ppos.mainTask.dynamicPriority = PRIORITY_DEFAULT; // Prioridade dinâmica da tarefa main
  ppos.mainTask.type = USER_TASK;                   // Tipo da tarefa main
  ppos.mainTask.status = READY;                     // Status da tarefa main
  ppos.mainTask.quanta = QUANTA_DEFAULT;            // Contador de quantum da tarefa main

  ppos.taskCounter = 1; // Inicializa o contador de tarefas

  ppos.readyQueue = NULL;              // Inicializa a fila de tarefas prontas
  ppos.currentTask = &(ppos.mainTask); // Tarefa corrente é a main

  task_init(&ppos.dispatcherTask, dispatcher, NULL); // Cria a tarefa dispatcher
  ppos.dispatcherTask.type = SYSTEM_TASK;            // Atribui o tipo da tarefa dispatcher como tarea do sistema

#if defined(DEBUG_PPOS_INIT) || defined(DEBUG_ALL)
  debug_print("(ppos_init) inicializando tratador de sinais.\n");
#endif

  // Inicializa o tratador de sinais do timer
  ppos.signalHandler.sa_handler = tick_handler; // Função de tratamento de sinais
  sigemptyset(&ppos.signalHandler.sa_mask);     // Máscara de sinais a serem bloqueados durante a execução do sinal
  ppos.signalHandler.sa_flags = 0;              // Opções de chamadas de sistema

  if (sigaction(SIGALRM, &ppos.signalHandler, 0) < 0)
  {
    perror("(ppos_init) erro ao instalar tratador de sinais:");
    exit(1);
  }

#if defined(DEBUG_PPOS_INIT) || defined(DEBUG_ALL)
  debug_print("(ppos_init) inicializando temporizador.\n");
#endif

  // Inicializa o temporizador
  ppos.clock = 0;                                    // Inicializa o clock do sistema
  ppos.timer.it_value.tv_usec = CLOCK_FIRST_SHOT_U;  // Primeiro disparo, em micro-segundos
  ppos.timer.it_value.tv_sec = CLOCK_FIRST_SHOT_S;   // Primeiro disparo, em segundos
  ppos.timer.it_interval.tv_usec = CLOCK_INTERVAL_U; // Disparos subsequentes, em micro-segundos
  ppos.timer.it_interval.tv_sec = CLOCK_INTERVAL_S;  // Disparos subsequentes, em segundos

  if (setitimer(ITIMER_REAL, &ppos.timer, 0) < 0)
  {
    perror("(ppos_init) erro ao iniciar temporizador:");
    exit(1);
  }

#if defined(DEBUG_PPOS_INIT) || defined(DEBUG_ALL)
  debug_print("(ppos_init) mainTaskId: %d\n", ppos.mainTask.id);
  debug_print("(ppos_init) currentTaskId: %d\n", ppos.currentTask->id);
  debug_print("(ppos_init) taskCounter: %d\n", ppos.taskCounter);
  debug_print("(ppos_init) sistema inicializado -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
#endif
}

// Inicializa uma nova tarefa. Retorna um ID > 0 ou erro.
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

  task->id = ppos.dispatcherTask.id + ppos.taskCounter; // ID da tarefa
  task->birthTime = systime();                          // Momento de criação da tarefa
  task->deathTime = 0;                                  // Momento de término da tarefa
  task->processorTime = 0;                              // Tempo de processador da tarefa
  task->activations = 0;                                // Número de ativações da tarefa
  task->exitCode = 0;                                   // Código de término da tarefa
  task->waitingQueue = NULL;                            // Fila de tarefas esperando por essa tarefa
  task->wakeTime = 0;                                   // Momento de acordar da tarefa

  task->type = USER_TASK;                   // Tipo da tarefa
  task->staticPriority = PRIORITY_DEFAULT;  // Prioridade estática da tarefa
  task->dynamicPriority = PRIORITY_DEFAULT; // Prioridade dinâmica da tarefa

  task->quanta = 0;                                            // Contador de quantum da tarefa
  task->status = READY;                                        // Status da tarefa
  queue_append((queue_t **)&ppos.readyQueue, (queue_t *)task); // Adiciona a tarefa na fila de prontas

#if defined(DEBUG_TASK_INIT) || defined(DEBUG_ALL)
  debug_print("(task_init) tarefa %d inicializada; birthTime: %d.\n", task->id, task->birthTime);
#endif

  ppos.taskCounter++; // Incrementa o contador de tarefas
  return task->id;
}

// Alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
  if (!task) // Se a tarefa não existir
  {
    perror("(task_switch) tarefa inexistente:");
    return -1;
  }

#if defined(DEBUG_TASK_SWITCH) || defined(DEBUG_ALL)
  if ((unsigned int)task_id() == ppos.mainTask.id)
  {
    if (task->id == ppos.dispatcherTask.id)
      debug_print("(task_switch) trocando de MAIN para DISPATCHER.\n");
    else
      debug_print("(task_switch) trocando de MAIN para %d.\n", task->id);
  }
  else if ((unsigned int)task_id() == ppos.dispatcherTask.id)
  {
    if (task->id == ppos.mainTask.id)
      debug_print("(task_switch) trocando de DISPATCHER para MAIN.\n");
    else
      debug_print("(task_switch) trocando de DISPATCHER para %d.\n", task->id);
  }
  else
  {
    if (task->id == ppos.dispatcherTask.id)
      debug_print("(task_switch) trocando de %d para DISPATCHER.\n", (unsigned int)task_id());
    else if (task->id == ppos.mainTask.id)
      debug_print("(task_switch) trocando de %d para MAIN.\n", (unsigned int)task_id());
    else
      debug_print("(task_switch) trocando de %d para %d.\n", (unsigned int)task_id(), task->id);
  }
#endif

  task_t *temp = ppos.currentTask; // Salva a tarefa atual

  task->activations++;           // Incrementa o contador de ativações da tarefa
  task->status = RUNNING;        // Atualiza o status da tarefa corrente
  task->quanta = QUANTA_DEFAULT; // Reinicia o contador de quantum

  ppos.currentTask = task; // Atualiza a tarefa corrente

  swapcontext(&temp->context, &task->context); // Troca de contexto

  return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit(int exit_code)
{
  if (exit_code < 0)
    printf("Tarefa %d terminada com erro: %d\n", ppos.currentTask->id, exit_code);

  ppos.taskCounter--;                      // Decrementa o contador de tarefas
  ppos.currentTask->status = TERMINATED;   // Atualiza o status da tarefa corrente
  ppos.currentTask->deathTime = systime(); // Atualiza o momento de término da tarefa corrente
  ppos.currentTask->exitCode = exit_code;  // Atualiza o código de término da tarefa corrente

  // every task in waitingQueue must be awaken
  task_t *tempTask = (task_t *)ppos.currentTask->waitingQueue;
  while (tempTask)
  {
    task_awake(tempTask, (task_t **)&ppos.currentTask->waitingQueue);
    tempTask = (task_t *)ppos.currentTask->waitingQueue;
  }

  printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
         ppos.currentTask->id,
         ppos.currentTask->deathTime - ppos.currentTask->birthTime,
         ppos.currentTask->processorTime,
         ppos.currentTask->activations);

  if (ppos.currentTask->id == ppos.dispatcherTask.id)
  {
#if defined(DEBUG_TASK_EXIT) || defined(DEBUG_ALL)
    debug_print("(task_exit) encerrando sistema.\n");
#endif
    exit(exit_code); // Encerra o sistema
  }

#if defined(DEBUG_TASK_EXIT) || defined(DEBUG_ALL)
  debug_print("(task_exit) tarefa %d finalizada.\n", ppos.currentTask->id);
  debug_print("(task_exit) quantidade de tarefas restantes: %d\n", ppos.taskCounter);
#endif

  task_switch(&ppos.dispatcherTask); // Volta para o dispatcher
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
  if (!ppos.currentTask) // Checa se existe uma tarefa corrente
  {
    perror("(task_id) tarefa inexistente:");
    return -1;
  }

  return ppos.currentTask->id; // Retorna o id da tarefa corrente
}

// Tarefa atual libera o processador
void task_yield()
{
  if (!ppos.currentTask) // Checa se existe uma tarefa corrente
  {
    perror("(task_yield) tarefa inexistente:");
    return;
  }

#if defined(DEBUG_TASK_YIELD) || defined(DEBUG_ALL)
  debug_print("(task_yield) tarefa %d liberou o processador.\n", ppos.currentTask->id);
#endif

  ppos.currentTask->status = READY;  // Atualiza o status da tarefa corrente
  task_switch(&ppos.dispatcherTask); // Troca de contexto
}

// Trata os sinais do temporizador do sistema
void tick_handler(int signum)
{

  ppos.clock++; // Incrementa o clock do sistema

#if defined(DEBUG_TICK_HANDLER) || defined(DEBUG_ALL)
  debug_print("(tick_handler) tarefa atual: %d - %d\n", ppos.currentTask->id, ppos.currentTask->quanta);
#endif

  // Checa se a tarefa corrente é do tipo sistema ou se o contador de quantum é maior que zero
  if (ppos.currentTask->type == SYSTEM_TASK || --ppos.currentTask->quanta > 0)
    return;

  task_yield();
}

// Dispatcher do sistema operacional
void dispatcher(void *arg)
{
#if defined(DEBUG_DISPATCHER) || defined(DEBUG_ALL)
  debug_print("(dispatcher) iniciando dispatcher.\n");
#endif
  task_t *currentTask = NULL;
  unsigned int taskProcessorTimeInit = 0, taskProcessorTimeEnd = 0;

  queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)&ppos.dispatcherTask); // Remove a tarefa dispatcher da fila de prontas
  ppos.taskCounter--;                                                          // Decrementa o contador de tarefas

  while (ppos.taskCounter > 0)
  {
    check_sleeping_tasks();    // Acorda as tarefas que já passaram do tempo de acordar
    currentTask = scheduler(); // Seleciona a próxima tarefa a ser executada

#if defined(DEBUG_DISPATCHER) || defined(DEBUG_ALL)
    debug_print("(dispatcher) próxima tarefa: %d\n", currentTask->id);
#endif

    if (currentTask)
    {
      queue_remove((queue_t **)&ppos.readyQueue, (queue_t *)currentTask);           // Remove a tarefa selecionada da fila de prontas
      taskProcessorTimeInit = systime();                                            // Inicia a contagem do tempo de processador
      task_switch(currentTask);                                                     // Troca de contexto para a tarefa selecionada
      taskProcessorTimeEnd = systime();                                             // Finaliza a contagem do tempo de processador
      currentTask->processorTime += (taskProcessorTimeEnd - taskProcessorTimeInit); // Atualiza o tempo de processador da tarefa selecionada

      // Trata o status da tarefa após a execução
      switch (currentTask->status)
      {
      case READY:
        queue_append((queue_t **)&ppos.readyQueue, (queue_t *)currentTask); // Adiciona a tarefa novamente na fila de prontas
        break;
      case TERMINATED:
        free(currentTask->context.uc_stack.ss_sp);  // Libera a pilha da tarefa
        currentTask->context.uc_stack.ss_sp = NULL; // Reseta o ponteiro da pilha
        currentTask->context.uc_stack.ss_size = 0;  // Reseta o tamanho da pilha
        currentTask = NULL;                         // Reseta o ponteiro da tarefa
        break;
      case SUSPENDED:
#if defined(DEBUG_DISPATCHER) || defined(DEBUG_ALL)
        debug_print("(dispatcher) status da tarefa %d: SUSPENSA\n", currentTask->id);
#endif
        break;
      default:
#if defined(DEBUG_DISPATCHER) || defined(DEBUG_ALL)
        debug_print("(dispatcher) status da tarefa %d: %d\n", currentTask->id, currentTask->status);
#endif
        break;
      }
    }
  }

#if defined(DEBUG_DISPATCHER) || defined(DEBUG_ALL)
  debug_print("(dispatcher) não há mais tarefas para executar.\n");
  debug_print("(dispatcher) encerrando dispatcher.\n");
#endif

  task_exit(0);
}

// Retorna a próxima tarefa a ser executada, ou NULL se não houver nada para fazer
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

#if defined(DEBUG_SCHEDULER) || defined(DEBUG_ALL)
  if (selectedTask)
  {
    debug_print("(scheduler) próxima tarefa a ser executada: %d.\n", selectedTask->id);
  }
  else
  {
    debug_print("(scheduler) não há tarefas para executar.\n");
  }
#endif

  return selectedTask; // Retorna a tarefa selecionada
}

// Define a prioridade estática de uma tarefa (ou a tarefa atual)
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

#if defined(DEBUG_TASK_SETPRIO) || defined(DEBUG_ALL)
  int id = task ? task->id : ppos.currentTask->id;
  debug_print("(task_setprio) prioridade da tarefa %d alterada para %d.\n", id, prio);
#endif

  if (task)
    task->staticPriority = prio; // Atribui a prioridade estática da tarefa passada
  else
    ppos.currentTask->staticPriority = prio; // Atribui a prioridade estática da tarefa corrente
}

// Retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
  if (!task && !ppos.currentTask) // Se a tarefa não existir e a tarefa corrente não existir
  {
    perror("(task_getprio) tarefa inexistente:");
    return -1;
  }

  return task ? task->staticPriority : ppos.currentTask->staticPriority; // Retorna a prioridade estática da tarefa passada ou da tarefa corrente
}

// Envelhece a prioridade dinâmica de uma tarefa
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

// Retorna o tempo atual do sistema
unsigned int systime()
{
  return ppos.clock;
}

// Retorna o tempo de criação da tarefa corrente
unsigned int task_birth_time()
{
  return ppos.currentTask->birthTime;
}

// Suspende a tarefa corrente e aguarda o encerramento de outra tarefa
int task_wait(task_t *task)
{
  if (!task) // Se a tarefa não existir
  {
    perror("(task_wait) tarefa inexistente:");
    return -1;
  }

  if (task->status != TERMINATED)                 // Se a tarefa estiver terminada
    task_suspend((task_t **)&task->waitingQueue); // Suspende a tarefa corrente e adiciona na fila de espera da tarefa passada

#if defined(DEBUG_TASK_WAIT) || defined(DEBUG_ALL)
  debug_print("(task_wait) tarefa %d aguardando término da tarefa %d.\n", ppos.currentTask->id, task->id);
#endif

  return task->exitCode;
}

// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend(task_t **queue)
{
  if (!queue) // Se a fila não existir
  {
    perror("(task_suspend) fila inexistente:");
    return;
  }

#if defined(DEBUG_TASK_SUSPEND) || defined(DEBUG_ALL)
  debug_print("(task_suspend) tarefa %d suspendendo.\n", ppos.currentTask->id);
#endif

  ppos.currentTask->status = SUSPENDED;                         // Atualiza o status da tarefa corrente
  queue_append((queue_t **)queue, (queue_t *)ppos.currentTask); // Adiciona a tarefa corrente na fila passada

  task_switch(&ppos.dispatcherTask); // Troca de contexto para o dispatcher
}

// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake(task_t *task, task_t **queue)
{
  if (!queue) // Se a fila não existir
  {
    perror("(task_awake) fila inexistente:");
    return;
  }

  if (!task) // Se a tarefa não existir
  {
    perror("(task_awake) tarefa inexistente:");
    return;
  }

  if (queue_remove((queue_t **)queue, (queue_t *)task) < 0) // Se houver erro ao remover a tarefa da fila passada
  {
    perror("(task_awake) erro ao acordar tarefa:");
    return;
  }

  task->status = READY;                                        // Atualiza o status da tarefa
  task->wakeTime = 0;                                          // Reseta o momento de acordar da tarefa
  queue_append((queue_t **)&ppos.readyQueue, (queue_t *)task); // Adiciona a tarefa na fila de prontas

#if defined(DEBUG_TASK_AWAKE) || defined(DEBUG_ALL)
  debug_print("(task_awake) tarefa %d acordada.\n", task->id);
#endif
}

// suspende a tarefa corrente por t milissegundos
void task_sleep(int t)
{
  if (t < 0) // Se o tempo for menor ou igual a zero
  {
    perror("(task_sleep) tempo inválido:");
    return;
  }

  ppos.currentTask->wakeTime = systime() + t;   // Atualiza o momento de acordar da tarefa corrente
  task_suspend((task_t **)&ppos.sleepingQueue); // Suspende a tarefa corrente e adiciona na fila de espera

#if defined(DEBUG_TASK_SLEEP) || defined(DEBUG_ALL)
  debug_print("(task_sleep) tarefa %d dormindo por %d ms.\n", ppos.currentTask->id, t);
#endif
}

// Acorda todas as tarefas dormindo que já passaram do tempo de acordar
void check_sleeping_tasks()
{
  task_t *nextTask, *wakeTask;

  if (queue_size((queue_t *)ppos.sleepingQueue) == 0) // Se a fila de tarefas dormindo estiver vazia não faz nada
    return;

  nextTask = (task_t *)ppos.sleepingQueue;

  do
  {
    if (nextTask->wakeTime <= systime()) // Se a tarefa já passou do tempo de acordar
    {
      wakeTask = nextTask; // Salva a tarefa a ser acordada

#if defined(DEBUG_TASK_SLEEP) || defined(DEBUG_ALL)
      debug_print("(check_sleeping_tasks) tarefa %d acordando.\n", wakeTask->id);
#endif

      nextTask = nextTask->next;                            // Avança para a próxima tarefa
      task_awake(wakeTask, (task_t **)&ppos.sleepingQueue); // Acorda a tarefa

      if (queue_size((queue_t *)ppos.sleepingQueue) == 0) // Se a fila de tarefas dormindo estiver vazia sai do loop
        break;
    }
    else
    {
      nextTask = nextTask->next; // Avança para a próxima tarefa
    }

  } while (nextTask != (task_t *)ppos.sleepingQueue); // Enquanto não percorrer toda a fila de tarefas dormindo
}