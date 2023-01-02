#include <stdio.h>

#include "board.h"

/* get information for making a move */
#define from(x)     (x & 63)
#define to(x)       ((x >> 6) & 63)
#define flags(x)    ((x >> 12) & 3)
#define promo(x)    ((x >> 14) & 3)

/* set information for making a move */
#define move(x, y, f, p)    (x | (y << 6) | (f << 12) | (p << 14))

/* get information for unmaking a move */
#define rule50(x)   (x & 63)
#define eptarget(x) ((x >> 6) & 63)
#define castling(x) ((x >> 12) & 15)

/* set information for unmaking a move */
#define undo(r, e, c)   (r | (e << 6) | (c << 12))

static const char pchars[] = " PNBRQK??pnbrqk";

/**
 * board_parsefen
 *  initialize the board based on the provided FEN string.
 */
void board_parsefen(Board *board, char *fen)
{
    char c, pos[128], side, castle[5], ep[3];
    int rule50, movenb;
    int i, rank, file, sq;
    Color color;
    Piece piece;

    rule50 = 0;
    movenb = 1;
    sscanf(fen, "%s %c %s %s %d %d", pos, &side, castle, ep, &rule50, &movenb);

    for (i = 0; i < 3; ++i)
        board->colorbb[i] = 0;
    for (i = 0; i < 7; ++i)
        board->piecebb[i] = 0;
    board->castling = NO_CASTLE;
    board->eptarget = 64;

    file = 0;
    rank = 7;
    for (i = 0; (c = pos[i]); ++i) {
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
    for(i = 0; (c = castle[i]); ++i) {
        switch (c) {
            case 'K':
                board->castling |= WHITE_OO;
                break;
            case 'Q':
                board->castling |= WHITE_OOO;
                break;
            case 'k':
                board->castling |= BLACK_OO;
                break;
            case 'q':
                board->castling |= BLACK_OOO;
                break;
        }
    }
    if (ep[0] != '-')
        board->eptarget = 8 * (ep[1] - '1') + (ep[0] - 'a');
    board->rule50 = rule50;
    board->movenb = movenb;
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

    printf("\n   +---+---+---+---+---+---+---+---+\n");
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
    printf("\nside: %c", board->side == WHITE ? 'w' : 'b');
    printf("\ncastling: ");
    if (board->castling == NO_CASTLE)
        printf("-");
    else {
        if (board->castling & WHITE_OO)
            printf("K");
        if (board->castling & WHITE_OOO)
            printf("Q");
        if (board->castling & BLACK_OO)
            printf("k");
        if (board->castling & BLACK_OOO)
            printf("q");
    }
    printf("\nen passant: ");
    if (board->eptarget == 64)
        printf("-");
    else
        printf("%d", board->eptarget);
    printf("\nhalfmove clock: %d", board->rule50);
    printf("\nfullmove number: %d\n\n", board->movenb);
}

/**
 * board_make
 *  Play the move, updating all relevant board information
 */
void board_make(Board *board, MoveInfo *minfo)
{
    U64 from, to;
    int flags, promo;
    Piece attacker, target;

    from = 1ull << from(minfo->move);
    to = 1ull << to(minfo->move);
    flags = flags(minfo->move);
    promo = promo(minfo->move);
    attacker = minfo->attacker;
    target = minfo->target;
    minfo->undo = undo(board->rule50, board->eptarget, board->castling);

    if (board->side == BLACK)
        ++board->movenb;

    if (attacker == PAWN || target != EMPTY)
        board->rule50 = 0;
    else
        ++board->rule50;

    board->piecebb[target] &= ~to;
    board->piecebb[attacker] ^= from | to;
    board->colorbb[board->side ^ 1] &= ~to;
    board->colorbb[board->side] ^= from | to;
    board->side ^= 1;

    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];
}

/**
 * board_unmake
 *  Undo the move, resetting all relevant board information
 */
void board_unmake(Board *board, MoveInfo *minfo)
{
    U64 from, to;
    int flags, promo, rule50, eptarget;
    Piece attacker, target;
    Castling castling;

    from = 1ull << from(minfo->move);
    to = 1ull << to(minfo->move);
    flags = flags(minfo->move);
    promo = promo(minfo->move);
    attacker = minfo->attacker;
    target = minfo->target;
    rule50 = rule50(minfo->undo);
    eptarget = eptarget(minfo->undo);
    castling = castling(minfo->undo);

    board->side ^= 1;
    board->colorbb[board->side] ^= from | to;
    board->colorbb[board->side ^ 1] |= to;
    board->piecebb[attacker] ^= from | to;
    board->piecebb[target] |= to;

    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];

    board->rule50 = rule50;

    if (board->side == BLACK)
        --board->movenb;
}
