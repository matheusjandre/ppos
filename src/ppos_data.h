// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos
// Alterações:
// - Adicionado constantes de tamanho das pilhas e de prioridades mínima, máxima, padrão e de envelhecimento
// - Adicionado constantes de tempo e quantum de tempo para as tarefas
// - Adicionado a estrutura de dados environment_t para armazenar as variáveis globais do sistema operacional
// - Modificado e adicionado status e prioridades a task_t
// - Adicionado a função dispatcher para gerenciar as tarefas
// - Adicionado a função scheduler para escolher a próxima tarefa a ser executada
// - Adicionado a função task_to_age para envelhecer a tarefa passada por parâmetro
// - Adicionado a função tick_handler para tratar os sinais do temporizador do sistema
// - Adicionado a função systime para retornar o tempo atual do sistema
// - Adicionado a função task_birth_time para retornar o tempo de criação da tarefa
// - Adicionado as funções task_wait, task_suspend, task_awake para manipular o estado das tarefas

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
#ifdef SLOW
#define CLOCK_FIRST_SHOT_U 2000
#define CLOCK_FIRST_SHOT_S 0
#define CLOCK_INTERVAL_U 100000
#define CLOCK_INTERVAL_S 0
#elif defined(FAST)
#define CLOCK_FIRST_SHOT_U 200
#define CLOCK_FIRST_SHOT_S 0
#define CLOCK_INTERVAL_U 100
#define CLOCK_INTERVAL_S 0
#else
#define CLOCK_FIRST_SHOT_U 2000
#define CLOCK_FIRST_SHOT_S 0
#define CLOCK_INTERVAL_U 1000
#define CLOCK_INTERVAL_S 0
#endif
#define QUANTA_DEFAULT 20

// Enum que define o estado de uma tarefa no operacional
typedef enum task_status_e
{
  READY,
  RUNNING,
  SUSPENDED,
  TERMINATED
} task_status_e;

// Enum que define o tipo de uma tarefa
typedef enum task_type_e
{
  USER_TASK,
  SYSTEM_TASK
} task_type_e;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next; // Ponteiros para usar na estrutura de filas
  unsigned int id;            // Identificador da tarefa
  task_type_e type;           // Tipo da tarefa (USER_TASK, SYSTEM_TASK)
  ucontext_t context;         // Contexto armazenado da tarefa
  task_status_e status;       // Pronta, rodando, suspensa, ...
  char staticPriority;        // Prioridade estatica
  char dynamicPriority;       // Prioridade dinamica
  unsigned char quanta;       // Contador de quantum
  unsigned int birthTime;     // Momento de criação
  unsigned int deathTime;     // Momento de término
  unsigned int processorTime; // Tempo de processador
  unsigned int activations;   // Número de ativações
  unsigned int exitCode;      // Código de término
  queue_t *waitingQueue;      // Fila de tarefas esperando por essa tarefa
} task_t;

typedef struct sigaction ppos_signal_handler_t; // Estrutura de tratadores de sinais
typedef struct itimerval ppos_timer_t;          // Estrutura de intervalo de tempo

// Estrutura que define as variáveis globais do sistema operacional
typedef struct
{
  unsigned int taskCounter;
  task_t mainTask, dispatcherTask, *currentTask;
  queue_t *readyQueue;
  ppos_signal_handler_t signalHandler;
  ppos_timer_t timer;
  unsigned int clock;
} ppos_environment_t;

// Estrutura que define um semáforo
typedef struct
{
  // Preencher quando necessário
} semaphore_t;

// Estrutura que define um mutex
typedef struct
{
  // Preencher quando necessário
} mutex_t;

// Estrutura que define uma barreira
typedef struct
{
  // Preencher quando necessário
} barrier_t;

// Estrutura que define uma fila de mensagens
typedef struct
{
  // Preencher quando necessário
} mqueue_t;

// Dispatcher do sistema operacional
void dispatcher(void *arg);

// Retorna a próxima tarefa a ser executada, ou NULL se não houver nada para fazer
task_t *scheduler();

// Rnvelhece a tarefa passada por parâmetro
int task_to_age(task_t *task);

// Trata os sinais do temporizador do sistema
void tick_handler(int signum);

// Retorna o tempo atual do sistema
unsigned int systime();

// Retorna o tempo atual do sistema
unsigned int task_birth_time();

#endif
