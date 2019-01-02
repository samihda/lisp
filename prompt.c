#include <stdio.h>

#define MAX_INPUT 2048

static char input[MAX_INPUT];

int main()
{
  puts("Ctrl+C to exit\n");

  while (1) {
    fputs("lisp> ", stdout);

    fgets(input, MAX_INPUT, stdin);

    printf("%s", input);
  }

  return 0;
}
