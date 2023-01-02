#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef uint16_t U16;
typedef uint64_t U64;

typedef enum colors Color;
typedef enum pieces Piece;
typedef enum castling Castling;

typedef struct board Board;
typedef struct move Move;

enum colors { WHITE, BLACK, BOTH };
enum pieces { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum castling {
    NO_CASTLE = 0,

    WHITE_OO = 1,
    BLACK_OO = 2,
    WHITE_OOO = 4,
    BLACK_OOO = 8,

    WHITE_CASTLE = WHITE_OO | WHITE_OOO,
    BLACK_CASTLE = BLACK_OO | BLACK_OOO,

    ALL_CASTLE = WHITE_CASTLE | BLACK_CASTLE
};

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
    Castling castling;
    int eptarget;
    int rule50;
    int movenb;
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
