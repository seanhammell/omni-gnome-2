#ifndef EVAL_H
#define EVAL_H

#include "board.h"

extern Tables tables;

void eval_init_pst(void);
int eval_heuristic(Board *board);

#endif  /* EVAL_H */
