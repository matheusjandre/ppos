#include <stdarg.h>
#include <stdio.h>
#include "dbug.h"

// Função para debugar
void debug_print(const char *format, ...)
{
  char buffer[256];              // Variavel para armazenar a string formatada
  char color[10] = "\033[1;34m"; // Variavel para armazenar o código de cor (Azul)
  char clear[10] = "\033[0m";    // Variavel para armazenar o código de cor (Padrão)

  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args); // Formata a string
  va_end(args);

  printf("%s%s%s", color, buffer, clear); // Printa a string formatada com a cor azul
}