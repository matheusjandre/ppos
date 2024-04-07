// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos
// Alterações:
// - Adicionado constantes de tamanho das pilhas e de prioridades mínima, máxima, padrão e de envelhecimento
// - Adicionado a estrutura de dados environment_t para armazenar as variáveis globais do sistema operacional
// - Modificado e adicionado status e prioridades a task_t
// - Adicionado a função dispatcher para gerenciar as tarefas
// - Adicionado a função scheduler para escolher a próxima tarefa a ser executada
// - Adicionado a função task_to_age para envelhecer a tarefa passada por parâmetro

// Estruturas de dados internas do sistema operacional
#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>           // biblioteca POSIX de trocas de contexto
#include <signal.h>             // biblioteca POSIX de sinais
#include <sys/time.h>           // biblioteca POSIX de temporizadores
#include "../lib/queue/queue.h" // biblioteca de filas

#define STACKSIZE 64 * 1024 // tamanho de pilha das threads

// Prioridades de tarefas
#define PRIORITY_MIN -20   // menor prioridade possível
#define PRIORITY_MAX 20    // maior prioridade possível
#define PRIORITY_DEFAULT 0 // prioridade padrão
#define PRIORITY_AGING -1  // valor de envelhecimento da prioridade

// Tempo e Quantum de tempo para as tarefas
#ifdef DEBUG
#define CLOCK_FIRST_SHOT_U 100
#define CLOCK_FIRST_SHOT_S 0
#define CLOCK_INTERVAL_U 100000
#define CLOCK_INTERVAL_S 0
#else
#define CLOCK_FIRST_SHOT_U 0
#define CLOCK_FIRST_SHOT_S 1
#define CLOCK_INTERVAL_U 1000
#define CLOCK_INTERVAL_S 0
#endif

#define QUANTA_DEFAULT 20

// Estrutura que define o estado de uma tarefa no operacional
typedef enum task_status_e
{
  READY,
  RUNNING,
  SUSPENDED,
  TERMINATED
} task_status_e;

typedef enum task_type_e
{
  USER_TASK,
  SYSTEM_TASK
} task_type_e;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next;   // ponteiros para usar em filas
  int id;                       // identificador da tarefa
  task_type_e type;             // tipo da tarefa (USER_TASK, SYSTEM_TASK)
  ucontext_t context;           // contexto armazenado da tarefa
  task_status_e status;         // pronta, rodando, suspensa, ...
  char staticPriority;          // prioridade estatica
  char dynamicPriority;         // prioridade dinamica
  unsigned char quantumCounter; // contador de quantum
} task_t;

typedef struct sigaction ppos_signal_handler_t;
typedef struct itimerval ppos_clock_t;

// Structure defining the global variables of the operating system
typedef struct
{
  int taskCounter;
  task_t mainTask, dispatcherTask, *currentTask;
  queue_t *readyQueue;
  ppos_signal_handler_t signalHandler;
  ppos_clock_t clock;
} ppos_environment_t;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t;

// dispatcher do sistema operacional
void dispatcher(void *arg);

// retorna a próxima tarefa a ser executada, ou NULL se não houver nada para fazer
task_t *scheduler();

// envelhece a tarefa passada por parâmetro
int task_to_age(task_t *task);

// trata os sinais do temporizador do sistema
void tick_handler(int signum);

#endif
