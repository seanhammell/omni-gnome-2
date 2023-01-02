#include <stdio.h>

#include "board.h"

/* information for making a move */
#define from(x)     (x & 63)
#define to(x)       ((x >> 6) & 63)
#define flags(x)    ((x >> 12) & 3)
#define promo(x)    ((x >> 14) & 3)

/* information for unmaking a move */
#define rule50(x)   (x & 63)
#define eptarget(x) ((x >> 6) & 63)
#define castling(x) ((x >> 12) & 15)

static const char pchars[] = " PNBRQK??pnbrqk";

/**
 * board_parsefen
 *  initialize the board based on the provided FEN string.
 */
void board_parsefen(Board *board, char *fen)
{
    char c, pos[128], side;
    int i, rank, file, sq;
    Color color;
    Piece piece;

    sscanf(fen, "%s %c", pos, &side);

    for (i = 0; i < 3; ++i)
        board->colorbb[i] = 0;
    for (i = 0; i < 7; ++i)
        board->piecebb[i] = 0;

    i = file = 0;
    rank = 7;
    while ((c = pos[i++])) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (c > '0' && c < '9') {
            file += c - '0';
        } else {
            color = (c - 'A') & 32 ? BLACK : WHITE;
            piece = 1; 
            while (c != pchars[piece])
                ++piece;
            piece &= 7;
            sq = 8 * rank + file;
            board->colorbb[color] |= 1ull << sq;
            board->piecebb[piece] |= 1ull << sq;
            ++file;
        }
    }
    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];
    board->side = side == 'w' ? WHITE : BLACK;
}

/**
 * board_display
 *  Print the current board position.
 */
void board_display(Board *board)
{
    int rank, file, sq;
    Color color;
    Piece piece;

    printf("   +---+---+---+---+---+---+---+---+\n");
    for (rank = 7; rank >= 0; --rank) {
        printf(" %d ", rank + 1);
        for (file = 0; file < 8; ++file) {
            sq = 8 * rank + file;
            printf("|");
            if (board->colorbb[BOTH] & (1ull << sq)) {
                color = board->colorbb[WHITE] & (1ull << sq) ? WHITE : BLACK;
                color *= 8;
                piece = 1;
                while (!(board->piecebb[piece] & (1ull << sq)))
                    ++piece;
                printf(" %c ", pchars[color + piece]);
            } else {
                printf("   ");
            }
        }
        printf("|\n   +---+---+---+---+---+---+---+---+\n");
    }
    printf("     a   b   c   d   e   f   g   h\n");
}

/**
 * board_make
 *  Play the move, updating all relevant board information
 */
void board_make(Board *board, Move *move)
{
    U64 from, to;
    int flags, promo;
    Piece attacker, target;

    from = 1ull << from(move->info);
    to = 1ull << to(move->info);
    flags = flags(move->info);
    promo = promo(move->info);
    attacker = move->attacker;
    target = move->target;

    board->piecebb[target] &= ~to;
    board->piecebb[attacker] ^= from | to;
    board->colorbb[board->side ^ 1] &= ~to;
    board->colorbb[board->side] ^= from | to;
    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];
    board->side ^= 1;
}
