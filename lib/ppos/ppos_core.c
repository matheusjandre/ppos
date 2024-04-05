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

  globals.taskCounter = 0;

  getcontext(&globals.mainTask.context);
  globals.mainTask.id = globals.taskCounter++;
  globals.currentTask = &(globals.mainTask);

#ifdef DEBUG
  printf("(ppos_init) sistema inicializado.\n");
  printf("(ppos_init) mainTaskId: %d\n", globals.mainTask.id);
  printf("(ppos_init) currentTaskId: %d\n", globals.currentTask->id);
  printf("(ppos_init) taskCounter: %d\n", globals.taskCounter);
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
  task->id = globals.taskCounter++;

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
  printf("(task_switch) trocando de %d para %d.\n", globals.currentTask->id, task->id);
#endif

  // Salva a tarefa atual
  task_t *temp = globals.currentTask;
  // Atualiza a tarefa corrente
  globals.currentTask = task;

  // Troca de contexto
  swapcontext(&temp->context, &task->context);
  return 0;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit(int exit_code)
{
  if (globals.currentTask->id == globals.mainTask.id)
  {
    exit(exit_code);
  }
  else
  {
    task_switch(&globals.mainTask);
  }
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

#ifdef DEBUG
  printf("(task_id) tarefa corrente: %d.\n", globals.currentTask->id);
#endif

  // Retorna o id da tarefa corrente
  return globals.currentTask->id;
}
