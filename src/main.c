#include <stdio.h>

/* https://tiswww.case.edu/php/chet/readline/rltop.html */
#include <readline/readline.h>
#include <readline/history.h>

#include "mpc.h"

enum { LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

typedef struct lval {
  int type;
  long num;
  char *sym;
  int count;
  struct lval **cell;
} lval;

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/**********************************************************************/

void lval_del(lval *v)
{
  switch (v->type) {
  case LVAL_NUM:
    break;

  case LVAL_SYM:
    free(v->sym);
    break;

  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }

    free(v->cell);
    break;
  }

  free(v);
}

/**********************************************************************/

void lval_print(lval *v);

void lval_expr_print(lval *v)
{
  putchar('(');

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count -1 )) {
      putchar(' ');
    }
  }

  putchar(')');
}

void lval_print(lval *v)
{
  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->num);
    break;

  case LVAL_SYM:
    printf("%s", v->sym);
    break;

  case LVAL_SEXPR:
    lval_expr_print(v);
    break;
  }
}

void lval_println(lval *v)
{
  lval_print(v);
  putchar('\n');
}

/**********************************************************************/

lval *lval_pop(lval *v, int i)
{
  lval *x = v->cell[i];

  memmove(&v->cell[i],
          &v->cell[i + 1],
          sizeof(lval*) * (v->count - i - 1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval *lval_take(lval *v, int i)
{
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval *builtin_op(lval *a, char *op)
{
  lval *x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval *y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) x->num += y->num;
    else if (strcmp(op, "-") == 0) x->num -= y->num;
    else if (strcmp(op, "*") == 0) x->num *= y->num;
    else if (strcmp(op, "/") == 0) x->num /= y->num;

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval *lval_eval(lval *v);

lval *lval_eval_sexpr(lval *v)
{
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  if (v->count == 0) return v;
  if (v->count == 1) return lval_take(v, 0);

  lval *f = lval_pop(v, 0);
  lval *result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval *lval_eval(lval *v)
{
  if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);
  return v;
}

/**********************************************************************/

lval *lval_read_num(mpc_ast_t *t)
{
  return lval_num(strtol(t->contents, NULL, 10));
}

lval *lval_add(lval *v, lval *x)
{
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

lval *lval_read(mpc_ast_t *t)
{
  if (strstr(t->tag, "number")) return lval_read_num(t);
  if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

  lval *x = NULL;

  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }

  if (strcmp(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) continue;
    if (strcmp(t->children[i]->contents, ")") == 0) continue;
    if (strcmp(t->children[i]->tag, "regex") == 0) continue;
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

/**********************************************************************/

int main()
{
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lisp = mpc_new("lisp");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                          \
              number : /-?[0-9]+/ ;                    \
              symbol : '+' | '-' | '*' | '/' ;         \
              sexpr  : '(' <expr>* ')' ;               \
              expr   : <number> | <symbol> | <sexpr> ; \
              lisp   : /^/ <expr>* /$/ ;               \
            ",
            Number, Symbol, Sexpr, Expr, Lisp);

  puts("Ctrl+C to exit\n");

  while (1) {
    char* input = readline("lisp> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin", input, Lisp, &r)) {
      lval *x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lisp);

  return 0;
}
