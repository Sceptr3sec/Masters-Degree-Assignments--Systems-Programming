#ifndef BC_H
#define BC_H

#include <unistd.h>  // write
#include <stdlib.h>  // malloc, free
#include <stdio.h>   // printf

typedef enum e_toktype {
    TOK_NUM = 1,
    TOK_OP  = 2,
    TOK_LP  = 3,
    TOK_RP  = 4
}   t_toktype;

typedef struct s_tok {
    t_toktype       type;
    long            value;   // for numbers
    char            op;      // '+', '-', '*', '/', '%', or 'n' (unary -), 'p' (unary +), '('
    int             prec;
    int             right_assoc; // 1 if right associative
    struct s_tok    *next;
}   t_tok;

typedef struct s_lstack {
    long                v;
    struct s_lstack     *next;
}   t_lstack;

/* utils */
int     my_strlen(const char *s);
void    err_write(const char *s);

/* token helpers */
t_tok   *tok_new_num(long v);
t_tok   *tok_new_op(char op, int prec, int right_assoc);
t_tok   *tok_new_paren(char p); /* '(' only used in operator stack */
void    tok_list_free(t_tok *lst);

/* shunting-yard + eval */
int     bc_shunting_yard(const char *expr, t_tok **out_rpn);
int     bc_eval_rpn(t_tok *rpn, long *result);

#endif