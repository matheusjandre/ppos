# PpOS

ppos is a toy operating system proposed by UFPR teacher Carlos Alberto Maziero as a project to understand the basics of an operating system

# Timeline

- [x] Biblioteca de filas
- [x] Trocas de contexto
- [x] Gestão de tarefas
- [x] Dispatcher
- [x] Escalonador por prioridades
- [x] Preempção por tempo
- [x] Contabilização
- [x] Tarefas suspensas
- [x] Tarefas dormindo
- [x] Semáforos
- [x] Uso de semáforos
- [ ] Operador Barreira
- [ ] Filas de mensagens
- [ ] Gerente de disco
- [ ] Escalonamento de disco

# Compiling

To compile the project, you need to have the following tools installed:

- `gcc`
- `make`

## Compiling and running

To compile and run the project, you can use the following commands:

```bash
# Compile the project
$ make
```

```bash
# Run the project
$ ./output
```

## Debugging

To debug the project, you can use the following commands:

```bash
# Compile the project with debug flags
$ make debug
```

### Debugging flags

- ALL: Print all debug flags
- PPOS_INIT: Print the initialization of the operating system
- TASK_INIT: Print the initialization of a task
- TASK_SWITCH: Print the task switch
- TASK_EXIT: Print the task exit
- TASK_YIELD: Print the task yield
- TICK_HANDLER: Print the tick handler
- DISPATCHER: Print the dispatcher
- SCHEDULER: Print the scheduler
- TASK_SETPRIO: Print the task set priority
- TASK_AWAKE: Print the task awake
- TASK_SUSPEND: Print the task suspend
- TASK_WAIT: Print the task wait
- TASK_SLEEP: Print the task sleep
- SEM_INIT: Print the semaphore initialization
- SEM_DOWN: Print the semaphore down
- SEM_UP: Print the semaphore up
- SEM_DESTROY: Print the semaphore destroy

The default debug flag is `ALL`, to change the debug flags, you can use the `DEBUG` variable:

```bash
# Compile the project with debug flags
$ make debug DEBUG="PPOS_INIT TASK_INIT DISPATCHER"
```

then normally run the project

```bash
# Run the project
$ ./output
```
