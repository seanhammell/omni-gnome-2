#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef uint64_t U64;
typedef struct board Board;

enum colors { WHITE, BLACK, BOTH };
enum pieces { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
};

void board_parsefen(Board *board, char *fen);

#endif  /* BOARD_H */
