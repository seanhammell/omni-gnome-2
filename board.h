#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef uint16_t U16;
typedef uint64_t U64;
typedef enum slider Slider;

typedef struct board Board;
typedef uint32_t Move;

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

enum slider { CROSS, DIAG };

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
    int castling;
    int eptarget;
    int rule50;
    int plynb;
    U16 undo[1024];

    U64 pins[2];
    U64 seen;
    U64 checkmask;
    int nchecks;
};

void board_inittables();

void board_parsefen(Board *board, char *fen);
void board_display(const Board *board);
void board_printmove(Move move);

int board_generate(Board *board, Move *movelist);

void board_make(Board *board, Move move);
void board_unmake(Board *board, Move move);

#endif  /* BOARD_H */
