#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define CHECKMATE   -30000
#define INF         60000

#define LSB(x)      __builtin_ctzll(x)
#define POPCNT(x)   __builtin_popcountll(x)

typedef uint16_t U16;
typedef uint64_t U64;

typedef struct tables Tables;
typedef struct board Board;
typedef uint32_t Move;

enum colors { WHITE, BLACK, BOTH };
enum pieces { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct tables {
    U64 bit[64];
    U64 rank_masks[64];
    U64 file_masks[64];
    U64 diag_masks[64];
    U64 anti_masks[64];

    U64 pawn_quiets[2][64];
    U64 pawn_caps[2][64];
    U64 knight_moves[64];
    U64 king_moves[64];
    U64 rays[8][256];

    int revoke_castling[64];
    U64 rook_switch[64];
};

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

    U64 targetmask;
};

static Tables tables;

void board_inittables();
void board_inithash();

void board_parsefen(Board *board, char *fen);
void board_parsemoves(Board *board, char *line);
void board_display(const Board *board);
void board_printmove(Move move);

int board_pullbit(U64 *bb);

U64 board_slide000(const Board *board, const int i);
U64 board_slide045(const Board *board, const int i);
U64 board_slide090(const Board *board, const int i);
U64 board_slide135(const Board *board, const int i);

int board_captures(Board *board, Move *movelist);
int board_generate(Board *board, Move *movelist);

void board_make(Board *board, Move move);
void board_unmake(Board *board, Move move);

int board_gameover(const Board *board, const int legal_moves);
int board_terminal(const Board *board, const int legal_moves, int depth);

#endif  /* BOARD_H */
