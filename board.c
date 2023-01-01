#include <stdio.h>

#include "board.h"

static const char ptypes[] = " PNBRQK??pnbrqk";

/**
 * board_parsefen
 *  initialize the board based on the provided FEN string.
 */
void board_parsefen(Board *board, char *fen)
{
    char c, pos[128], side;
    int i, rank, file, sq;
    int color, type;

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
            type = 1; 
            while (c != ptypes[type])
                ++type;
            type &= 7;
            sq = 8 * rank + file;
            board->colorbb[color] |= 1ull << sq;
            board->piecebb[type] |= 1ull << sq;
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
    int color, type;

    printf("   +---+---+---+---+---+---+---+---+\n");
    for (rank = 7; rank >= 0; --rank) {
        printf(" %d ", rank + 1);
        for (file = 0; file < 8; ++file) {
            sq = 8 * rank + file;
            printf("|");
            if (board->colorbb[BOTH] & (1ull << sq)) {
                color = board->colorbb[WHITE] & (1ull << sq) ? WHITE : BLACK;
                color *= 8;
                type = 1;
                while (!(board->piecebb[type] & (1ull << sq)))
                    ++type;
                printf(" %c ", ptypes[color + type]);
            } else {
                printf("   ");
            }
        }
        printf("|\n   +---+---+---+---+---+---+---+---+\n");
    }
    printf("     a   b   c   d   e   f   g   h\n");
}
