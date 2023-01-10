#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef uint16_t U16;
typedef uint64_t U64;

typedef struct board Board;
typedef uint32_t Move;

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
    int castling;
    int eptarget;
    int rule50;
    int plynb;
    U16 undo[1024];
    U64 history[1024];

    U64 pins[2];
    U64 seen;
    U64 checkmask;
    int nchecks;

    int root_side;
};

void board_inittables();
void board_inithash();

void board_parsefen(Board *board, char *fen);
void board_parsemoves(Board *board, char *line);
void board_display(const Board *board);
void board_printmove(Move move);

int board_generate(Board *board, Move *movelist);

void board_make(Board *board, Move move);
void board_unmake(Board *board, Move move);

int board_gameover(const Board *board, int legalmoves);
int board_evaluate(const Board *board, int legalmoves);

#endif  /* BOARD_H */
