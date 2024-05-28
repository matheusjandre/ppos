// GRR20215397 Matheus Henrique Jandre Ferraz
// Projeto no github: https://github.com/matheusjandre/ppos

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "src/ppos.h"

#define VACANCY_SIZE 5

typedef struct item_t
{
   struct item_t *next;
   struct item_t *prev;
   int value;
} item_t;

task_t p1, p2, p3, c1, c2;
item_t *buffer;
semaphore_t s_buffer, s_vacancy, s_item;

#ifdef DEBUG_BUFFER
void print_elem(void *ptr)
{
   item_t *item = ptr;

   if (!item)
      return;

   debug_print("%2d", item->value);
}
#endif

// tarefa produtora
void Producer(void *arg)
{
   while (1)
   {
      item_t *item = malloc(sizeof(item_t));

      task_sleep(1000);

      item->next = NULL;
      item->prev = NULL;
      item->value = rand() % 100;

      sem_down(&s_vacancy);
      sem_down(&s_buffer);

      printf("%s produziu %d\n", (char *)arg, item->value);
      queue_append((queue_t **)&buffer, (queue_t *)item);

#ifdef DEBUG_BUFFER
      queue_print("Buffer:  ", (queue_t *)buffer, print_elem);
      printf("\n");
#endif

      sem_up(&s_buffer);
      sem_up(&s_item);
   }
}

// tarefa consumidora
void Consumer(void *arg)
{
   while (1)
   {
      sem_down(&s_item);
      sem_down(&s_buffer);

      item_t *item = (item_t *)buffer;
      queue_remove((queue_t **)&buffer, (queue_t *)item);

      sem_up(&s_buffer);
      sem_up(&s_vacancy);

      printf("%s consumiu %d\n", (char *)arg, item->value);
      free(item);
      task_sleep(1000);
   }
}

int main(int argc, char *argv[])
{
   unsigned int seed = (unsigned int)getpid();
   srand(seed);

   sem_init(&s_buffer, 1);
   sem_init(&s_vacancy, VACANCY_SIZE);
   sem_init(&s_item, 0);

   printf("main: inicio\n");

   ppos_init();

   task_init(&p1, Producer, "p1");
   task_init(&c1, Consumer, "                             c1");
   task_init(&p2, Producer, "p2");
   task_init(&c2, Consumer, "                             c2");
   task_init(&p3, Producer, "p3");

   printf("main: fim\n");
   task_exit(0);
}