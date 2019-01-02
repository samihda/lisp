#include <stdio.h>
#include <stdlib.h>

/* https://tiswww.case.edu/php/chet/readline/rltop.html */
#include <readline/readline.h>
#include <readline/history.h>

int main()
{
  puts("Ctrl+C to exit\n");

  while (1) {
    char* input = readline("lisp> ");

    add_history(input);

    printf("%s\n", input);

    free(input);
  }

  return 0;
}
