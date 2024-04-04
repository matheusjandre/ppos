// GRR20215397 Matheus Henrique Jandre Ferraz

#include <stdio.h>
#include "queue.h"

int queue_size(queue_t *queue)
{
   queue_t *temp = queue;
   int c = 0;

   do
   {
      if (!temp) // Se fila vazia sai do loop sem incrementar
         break;

      c++;
      temp = temp->next;

   } while (temp != queue); // Enquanto não volta para a cabeça

   return c;
}

void queue_print(char *name, queue_t *queue, void print_elem(void *))
{
   queue_t *temp = queue;

   printf("%s: [", name);

   do
   {
      if (!queue) // Se fila vazia, não imprime nada entre os colchetes
         break;

      print_elem(temp); // Chama a funcão de impressão para o elemento
      temp = temp->next;

      if (temp != queue)
         printf(" ");

   } while (temp != queue); // Enquanto não volta para a cabeça

   printf("]\n");
}

int queue_append(queue_t **queue, queue_t *elem)
{
   queue_t *temp;

   if (!queue)
   {
      perror("Fila inexistente.");
      return -1;
   }

   if (!elem)
   {
      perror("Elemento inexistente.");
      return -2;
   }

   if (elem->next != NULL || elem->prev != NULL)
   {
      perror("O elemento pertence a uma fila.");
      return -3;
   }

   if (!*queue) // Se fila vazia
   {
      *queue = elem; // Cabeça aponta para o elemento

      elem->next = elem; // NEXT: Novo elemento = novo elemento
      elem->prev = elem; // PREV: Novo elemento = novo elemento

      return 0; // Elemento inserido com sucesso
   }

   temp = (*queue)->prev; // Ultimo elemento

   temp->next = elem;     // NEXT: Ultimo elemento = novo elemento
   elem->prev = temp;     // PREV: Novo elemento = último elemento
   elem->next = *queue;   // NEXT: Novo elemento = cabeça
   (*queue)->prev = elem; // PREV: Cabeça = novo elemento

   return 0; // Elemento inserido com sucesso
}

int queue_remove(queue_t **queue, queue_t *elem)
{
   queue_t *remove;

   if (!queue)
   {
      perror("Fila inexistente.");
      return -1;
   }

   if (!(*queue))
   {
      perror("A fila está vazia.");
      return -2;
   }

   if (!elem)
   {
      perror("Elemento inexistente.");
      return -3;
   }

   remove = *queue; // Inicia a buscando pelo elemento da cabeça

   do
   {
      if (remove == elem) // Se o elemento a ser removido for a cabeça
         break;           // Sai do loop

      remove = remove->next; // Próximo elemento

   } while (remove != *queue && remove != elem); // Enquanto não volta para a cabeça ou encontra o elemento

   if (remove != elem) // Se não encontrou o elemento
   {
      perror("O elemento não foi encontrado na fila.");
      return -4;
   }

   if (remove->next == remove && remove->prev == remove) // Se for o único elemento da fila
   {
      remove->next = NULL; // Limpa o ponteiro para o próximo
      remove->prev = NULL; // Limpa o ponteiro para o anterior
      *queue = NULL;       // Fila vazia
      return 0;            // Elemento removido com sucesso
   }

   if (remove == *queue) // Se for o primeiro elemento mas não o único
   {
      *queue = remove->next; // Cabeça aponta para o próximo
   }

   remove->prev->next = remove->next; // NEXT: Elemento anterior = próximo do elemento a ser removido
   remove->next->prev = remove->prev; // PREV: Próximo elemento = anterior do elemento a ser removido
   remove->next = NULL;               // Limpa o ponteiro para o próximo
   remove->prev = NULL;               // Limpa o ponteiro para o anterior

   return 0;
}