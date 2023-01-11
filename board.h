#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define CHECKMATE   -200000
#define INF         600000

#define LSB(x)      __builtin_ctzll(x)
#define POPCNT(x)   __builtin_popcountll(x)

typedef uint32_t U32;
typedef uint64_t U64;

typedef struct board Board;
typedef uint32_t Move;

enum colors { WHITE, BLACK, BOTH };
enum pieces { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct board {
    U64 colorbb[3];
    U64 piecebb[7];
    int side;
    int castling;
    int eptarget;
    int rule50;
    int quiet;
    int plynb;
    U32 undo[1024];
    U64 history[1024];

    U64 pins[2];
    U64 seen;
    U64 checkmask;
    int nchecks;
};

void board_inittables();
void board_inithash();

void board_parsefen(Board *board, char *fen);
void board_parsemoves(Board *board, char *line);
void board_display(const Board *board);
void board_printmove(Move move);

int board_pullbit(U64 *bb);
int board_generate(Board *board, Move *movelist);

void board_make(Board *board, Move move);
void board_unmake(Board *board, Move move);

int board_gameover(const Board *board, const int legal_moves);
int board_terminal(const Board *board, const int legal_moves);

#endif  /* BOARD_H */
