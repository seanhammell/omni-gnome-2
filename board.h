#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef uint16_t U16;
typedef uint64_t U64;
typedef enum colors Color;
typedef enum pieces Piece;
typedef struct board Board;
typedef struct move Move;

enum colors { WHITE, BLACK, BOTH };
enum pieces { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
};

struct move {
    U16 info;
    Piece attacker;
    Piece target;
};

void board_parsefen(Board *board, char *fen);
void board_display(Board *board);
void board_make(Board *board, Move *move);

#endif  /* BOARD_H */
