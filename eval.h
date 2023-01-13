#ifndef EVAL_H
#define EVAL_H

#include "board.h"

extern Tables tables;

int eval_heuristic(const Board *board);

#endif  /* EVAL_H */
