#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"

#define MOVE_WHITE_OO   0xc6184
#define MOVE_WHITE_OOO  0xc6084
#define MOVE_BLACK_OO   0xc6fbc
#define MOVE_BLACK_OOO  0xc6ebc

#define CASTLE_RIGHTS(b)    (b->castling & (WHITE_CASTLE << b->side))
#define RIGHTS_OO(b)        (b->castling & (WHITE_OO << b->side))
#define RIGHTS_OOO(b)       (b->castling & (WHITE_OOO << b->side))
#define OPEN_OO(b)          (!(96ull << (56 * b->side) & b->colorbb[BOTH]))
#define OPEN_OOO(b)         (!(14ull << (56 * b->side) & b->colorbb[BOTH]))
#define SAFE_OO(b)          (!(96ull << (56 * b->side) & b->seen))
#define SAFE_OOO(b)         (!(12ull << (56 * b->side) & b->seen))
#define ROOK_SWITCH_OO(b)   (160ull << (56 * b->side))
#define ROOK_SWITCH_OOO(b)  (9ull << (56 * b->side))

#define ENPOFF(b)   (b->eptarget + (b->side == WHITE ? -8 : 8))

#define FROM(x)     (x & 63)
#define TO(x)       ((x >> 6) & 63)
#define ATTACKER(x) ((x >> 12) & 7)
#define TARGET(x)   ((x >> 15) & 7)
#define FLAGS(x)    ((x >> 18) & 3)
#define PROMO(x)    (((x >> 20) & 3) + KNIGHT)

#define FROM_(x)        (x)
#define TO_(x)          (x << 6)
#define ATTACKER_(x)    (x << 12)
#define TARGET_(x)      (x << 15)
#define FLAGS_(x)       (x << 18)
#define PROMO_(x)       ((x - KNIGHT) << 20)

#define RULE50(x)   (x & 63)
#define ENP(x)      ((x >> 6) & 63)
#define CASTLE(x)   ((x >> 12) & 15)

#define RULE50_(x)  (x)
#define ENP_(x)     (x << 6)
#define CASTLE_(x)  (x << 12)

#define FLAG_HASH(s, e, c)   (s | ((e % 15) << 4) | (c << 8))

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
enum flags { NO_FLAG, PROMOTION, ENPASSANT, CASTLING };
enum slider { CROSS, DIAG };

static const char pchars[] = " PNBRQK??pnbrqk";
static const U64 HASH = 2859235813007982802;
static U64 hashxor[512];

/**
 * rankmask
 *  compute the rank mask for the given square
 */
U64 rankmask(const int i)
{
    const U64 RANK_1 = 0xff;
    return RANK_1 << (i & 56);
}

/**
 * filemask
 *  compute the file mask for the given square
 */
U64 filemask(const int i)
{
    const U64 FILE_A = 0x101010101010101;
    return FILE_A << (i & 7);
}

/**
 * diagmask
 *  compute the diagonal mask for the given square
 */
U64 diagmask(const int i)
{
    const U64 DIAG = 0x8040201008040201;
    const int diag = 8 * (i & 7) - (i & 56);
    const int north = -diag & (diag >> 31);
    const int south = diag & (-diag >> 31);
    return (DIAG >> south) << north;
}

/**
 * antimask
 *  compute the anti-diagonal mask for the given square
 */
U64 antimask(const int i)
{
    const U64 DIAG = 0x0102040810204080;
    const int diag = 56 - 8 * (i & 7) - (i & 56);
    const int north = -diag & (diag >> 31);
    const int south = diag & (-diag >> 31);
    return (DIAG >> south) << north;
}

/**
 * initmasks
 *  initialize line masks for each square in each direction
 */
void initmasks()
{
    int i;

    for (i = 0; i < 64; ++i) {
        tables.bit[i] = 1ull << i;
        tables.rank_masks[i] = rankmask(i);
        tables.file_masks[i] = filemask(i);
        tables.diag_masks[i] = diagmask(i);
        tables.anti_masks[i] = antimask(i);
    }
}

/**
 * initpawns
 *  intitialize the pawn quiet and capture moves lookup tables
 */
void initpawns()
{
    int i;
    U64 bit;

    for (i = 0, bit = 1; i < 64; ++i, bit <<= 1) {
        tables.pawn_quiets[0][i] = bit << 8;
        tables.pawn_quiets[1][i] = bit >> 8;
        tables.pawn_caps[0][i] = 0;
        tables.pawn_caps[1][i] = 0;

        if (!(bit & tables.file_masks[0])) {
            tables.pawn_caps[0][i] |= bit << 7;
            tables.pawn_caps[1][i] |= bit >> 9;
        }

        if (!(bit & tables.file_masks[7])) {
            tables.pawn_caps[0][i] |= bit << 9;
            tables.pawn_caps[1][i] |= bit >> 7;
        }
    }
}

/**
 * initknights
 *  initialize the knight moves lookup table
 */
void initknights()
{
    int i;
    U64 bit, east, west, set, moves;

    for (i = 0, bit = 1; i < 64; ++i, bit <<= 1) {
        moves = 0;
        east = bit & tables.file_masks[7] ? 0 : bit << 1;
        west = bit & tables.file_masks[0] ? 0 : bit >> 1;
        set = east | west;
        moves |= set << 16 | set >> 16;
        east = bit & tables.file_masks[6] ? 0 : east << 1;
        west = bit & tables.file_masks[1] ? 0 : west >> 1;
        set = east | west;
        moves |= set << 8 | set >> 8;
        tables.knight_moves[i] = moves;
    }
}

/**
 * initkings
 *  initialize the king moves lookup table
 */
void initkings()
{
    int i;
    U64 bit, east, west, set, moves;

    for (i = 0, bit = 1; i < 64; ++i, bit <<= 1) {
        east = bit & tables.file_masks[7] ? 0 : bit << 1;
        west = bit & tables.file_masks[0] ? 0 : bit >> 1;
        set = east | bit | west;
        moves = set << 8 | set | set >> 8;
        moves ^= bit;
        tables.king_moves[i] = moves;
    }
}

/**
 * initrays
 *  initialize the ray lookup table for kindergarten bitboard calculations
 */
void initrays()
{
    int bit, i, occ, left, right;
    U64 ray;

    for (i = 0, bit = 1; i < 8; ++i, bit <<= 1) {
        for (occ = 0; occ < 256; ++occ) {
            ray = 0;
            left = bit >> 1;
            while (left) {
                ray |= left;
                if (left & occ)
                    break;
                left >>= 1;
            }
            right = bit << 1;
            while (right < 256) {
                ray |= right;
                if (right & occ)
                    break;
                right <<= 1;
            }
            tables.rays[i][occ] = ray;
        }
    }
}

void initcastling()
{
    int i;

    for (i = 0; i < 64; ++i) {
        tables.revoke_castling[i] = ALL_CASTLE;
        tables.rook_switch[i] = 0;
    }

    tables.revoke_castling[0] ^= WHITE_OOO;
    tables.rook_switch[2] = 9ull;
    tables.revoke_castling[4] ^= WHITE_CASTLE;
    tables.rook_switch[6] = 160ull;
    tables.revoke_castling[7] ^= WHITE_OO;
    tables.revoke_castling[56] ^= BLACK_OOO;
    tables.rook_switch[58] = 648518346341351424ull;
    tables.revoke_castling[60] ^= BLACK_CASTLE;
    tables.rook_switch[62] = 11529215046068469760ull;
    tables.revoke_castling[63] ^= BLACK_OO;
}

/**
 * board_inittables
 *  Call table initialization functions
 */
void board_inittables()
{
    initmasks();
    initpawns();
    initknights();
    initkings();
    initrays();
    initcastling();
}

/**
 * board_inithash
 *  Initialize hashxor table with random U64 values
 */
void board_inithash()
{
    int i;
    U64 n;

    for (i = 0, n = 1; i < 512; ++i) {
        n ^= HASH;
        ++n;
        hashxor[i] = n;
    }
}

/**
 * hashposition
 *  calculate a value for the current position using the hash table
 */
U64 hashposition(const Board *board)
{
    int i, j;
    U64 key, position;

    key = 0;
    for (i = 0; i < 2; ++i) {
        for (j = 1; j < 7; ++j) {
            position = board->piecebb[j] & board->colorbb[i];
            position ^= hashxor[8 * i + j];
            key ^= position;
        }
    }
    i = FLAG_HASH(board->side, board->eptarget, board->castling);
    key ^= hashxor[i];
    return key;
}

/**
 * board_parsefen
 *  initialize the board based on the provided FEN string.
 */
void board_parsefen(Board *board, char *fen)
{
    char c, pos[128], side, castle[5], ep[3];
    int rule50;
    int i, rank, file, color, piece;
    U64 sq;

    rule50 = 0;
    sscanf(fen, "%s %c %s %s %d", pos, &side, castle, ep, &rule50);

    for (i = 0; i < 3; ++i)
        board->colorbb[i] = 0;
    for (i = 0; i < 7; ++i)
        board->piecebb[i] = 0;
    board->castling = NO_CASTLE;
    board->eptarget = 0;

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
            sq = tables.bit[8 * rank + file];
            board->colorbb[color] |= sq;
            board->piecebb[piece] |= sq;
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
    board->plynb = 0;
    board->history[board->plynb] = hashposition(board);
    board->capmask = ~0ull;
}

/**
 * board_parsemove
 *  play the specified string of moves in the current position
 */
void board_parsemoves(Board *board, char *line)
{
    int i, rank, file;
    int from, to, attacker, target, flags, promo;
    Move move;

    while (*line == ' ')
        ++line;
    while (*line != '\n') {
        file = *line - 'a';
        rank = *(line + 1) - '1';
        from = 8 * rank + file;

        file = *(line + 2) - 'a';
        rank = *(line + 3) - '1';
        to = 8 * rank + file;

        for (i = 0; i < 7; ++i) {
            if (board->piecebb[i] & tables.bit[from])
                attacker = i;
            if (board->piecebb[i] & tables.bit[to])
                target = i;
        }
        move = FROM_(from) | TO_(to) | ATTACKER_(attacker) | TARGET_(target);
        line += 4;

        flags = 0;
        promo = 0;
        if (*line != ' ' && *line != '\n') {
            i = 0;
            while (*line != pchars[i])
                ++i;
            flags = PROMOTION;
            promo = i;
            ++line;
        } else if (board->eptarget && to == board->eptarget) {
            flags = ENPASSANT;
        } else if (attacker == KING && 
                   (tables.bit[from] ^ tables.bit[to]) % 10 == 0) {
            flags = CASTLING;
        }

        move |= FLAGS_(flags) | PROMO_(promo);
        board_make(board, move);
        while (*line == ' ')
            ++line;
    }
}

/**
 * board_display
 *  Print the current board position.
 */
void board_display(const Board *board)
{
    int rank, file, color, piece;
    U64 sq;

    printf("\n   +---+---+---+---+---+---+---+---+\n");
    for (rank = 7; rank >= 0; --rank) {
        printf(" %d ", rank + 1);
        for (file = 0; file < 8; ++file) {
            sq = tables.bit[8 * rank + file];
            printf("|");
            if (board->colorbb[BOTH] & sq) {
                color = board->colorbb[WHITE] & sq ? WHITE : BLACK;
                color *= 8;
                piece = 1; 
                while (!(board->piecebb[piece] & sq))
                    ++piece;
                piece &= 7;
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
    if (board->eptarget == 0)
        printf("-");
    else
        printf("%d", board->eptarget);
    printf("\nhalfmove clock: %d", board->rule50);
    printf("\nply: %d\n\n", board->plynb);
}

/**
 * board_printmove
 *  print the move
 */
void board_printmove(Move move)
{
    int from, to, flags, promo;

    from = FROM(move);
    to = TO(move);
    flags = FLAGS(move);
    promo = PROMO(move);

    printf("%c%c", 'a' + (from % 8), '1' + (from / 8));
    printf("%c%c", 'a' + (to % 8), '1' + (to / 8));
    if (flags == PROMOTION)
        printf("%c", pchars[promo + 8]);
}

/**
 * board_pullbit
 *  pop the lsb from the board and return it's index
 */
int board_pullbit(U64 *bb)
{
    const int i = LSB(*bb);
    *bb &= *bb - 1;
    return i;
}

/**
 * board_slide000
 *  compute horizontal sliding attacks
 */
U64 board_slide000(const Board *board, const int i)
{
    U64 occ, ray;

    occ = tables.rank_masks[i] & board->colorbb[BOTH];
    occ *= tables.file_masks[0];
    occ >>= 56;
    ray = tables.rays[i & 7][occ] * tables.file_masks[0];
    ray &= tables.rank_masks[i];
    return ray;
}

/**
 * board_slide045
 *  compute diagonal sliding attacks
 */
U64 board_slide045(const Board *board, const int i)
{
    U64 occ, ray;

    occ = tables.diag_masks[i] & board->colorbb[BOTH];
    occ *= tables.file_masks[0];
    occ >>= 56;
    ray = tables.rays[i & 7][occ] * tables.file_masks[0];
    ray &= tables.diag_masks[i];
    return ray;
}

/**
 * board_slide090
 *  compute file sliding attacks
 */
U64 board_slide090(const Board *board, const int i)
{
    U64 occ, ray;

    occ = tables.file_masks[i] & board->colorbb[BOTH];
    occ >>= i & 7;
    occ *= tables.diag_masks[0];
    occ >>= 56;
    ray = tables.rays[(i ^ 56) >> 3][occ] * tables.diag_masks[0];
    ray &= tables.file_masks[7];
    ray >>= (i & 7) ^ 7;
    return ray;
}

/**
 * board_slide135
 *  compute anti-diagonal sliding attacks
 */
U64 board_slide135(const Board *board, const int i)
{
    U64 occ, ray;

    occ = tables.anti_masks[i] & board->colorbb[BOTH];
    occ *= tables.file_masks[0];
    occ >>= 56;
    ray = tables.rays[i & 7][occ] * tables.file_masks[0];
    ray &= tables.anti_masks[i];
    return ray;
}

/**
 * pins
 *  compute the pinned pieces along the given ray
 */
void pins(Board *board, U64 (*slide)(const Board *, int), const int slider)
{
    int i;
    U64 sliders, pin;
    U64 enemyray;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[slider] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    i = LSB(king);
    const U64 kingray = slide(board, i);
    while(sliders) {
        i = board_pullbit(&sliders);
        enemyray = slide(board, i);
        pin = kingray & enemyray & board->colorbb[board->side];
        if (pin)
            board->pins[slider % 2] |= (slide(board, LSB(pin)) | pin) ^ king;
    }
}

/**
 * seenray
 *  compute the squares seen by the enemy along the given ray
 */
void seenray(Board *board, U64 (*slide)(const Board *, int), const int slider)
{
    int i;
    U64 sliders;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[slider] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    board->colorbb[BOTH] ^= king;
    while(sliders) {
        i = board_pullbit(&sliders);
        board->seen |= slide(board, i);
    }
    board->colorbb[BOTH] ^= king;
}

/**
 * checkray
 *  compute the check mask along the given ray
 */
void checkray(Board *board, U64 (*slide)(const Board *, int), const int slider)
{
    int i;
    U64 sliders;
    U64 enemyray;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[slider] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    i = LSB(king);
    const U64 kingray = slide(board, i);
    sliders &= kingray;
    while(sliders) {
        i = board_pullbit(&sliders);
        enemyray = slide(board, i);
        board->checkmask |= (kingray & enemyray) | tables.bit[i];
        ++board->nchecks;
    }
}

/**
 * pawndanger
 *  compute the squares seen and checks given by enemy pawns
 */
void pawndanger(Board *board)
{
    int i;
    U64 pawns;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    i = LSB(king);
    const U64 checks = tables.pawn_caps[board->side][i] & pawns;
    board->checkmask |= checks;
    board->nchecks += POPCNT(checks);
    while (pawns) {
        i = board_pullbit(&pawns);
        board->seen |= tables.pawn_caps[board->side ^ 1][i];
    }
}

/**
 * knightdanger
 *  compute the squares seen and checks given by enemy knights
 */
void knightdanger(Board *board)
{
    int i;
    U64 knights;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    knights = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];

    i = LSB(king);
    const U64 checks = tables.knight_moves[i] & knights;
    board->checkmask |= checks;
    board->nchecks += POPCNT(checks);
    while (knights) {
        i = board_pullbit(&knights);
        board->seen |= tables.knight_moves[i];
    }
}

/**
 * kingdanger
 *  compute the squares seen by the enemy king
 */
void kingdanger(Board *board)
{
    int i;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side ^ 1];
    board->seen |= tables.king_moves[LSB(king)];
}

/**
 * setdanger
 *  find pinned pieces, squares seen by the enemy, and checks
 */
void setdanger(Board *board)
{
    int i;

    const int sliders[4] = {ROOK, BISHOP, ROOK, BISHOP};
    U64 (* const slide[4])(const Board *, const int) = {
        board_slide000, board_slide045, board_slide090, board_slide135
    };

    board->pins[CROSS] = board->pins[DIAG] = 0;
    board->seen = 0;
    board->checkmask = 0;
    board->nchecks = 0;

    for (i = 0; i < 4; ++i) {
        pins(board, slide[i], sliders[i]);
        seenray(board, slide[i], sliders[i]);
        checkray(board, slide[i], sliders[i]);
    }

    pawndanger(board);
    knightdanger(board);
    kingdanger(board);

    if (!board->checkmask)
        board->checkmask = ~board->checkmask;
}

/**
 * pawnquiets
 *  return a bitboard of pawn pushes from the given square
 */
U64 pawnquiets(const Board *board, const int i)
{
    int dblpush;
    U64 targets;

    targets = tables.pawn_quiets[board->side][i];
    targets &= board->piecebb[EMPTY];
    if (targets && ((i & 56) == 8 || (i & 56) == 48)) {
        dblpush = board->side == WHITE ? i + 8 : i - 8;
        targets |= tables.pawn_quiets[board->side][dblpush];
        targets &= board->piecebb[EMPTY];
    }
    targets &= board->checkmask;
    return targets;
}

/**
 * pawncaps
 *  return a bitboard of pawn captures from the given square
 */
U64 pawncaps(const Board *board, const int i)
{
    U64 enemies, checkmask, targets;

    enemies = board->colorbb[board->side ^ 1];
    checkmask = board->checkmask;
    if (board->eptarget)
        enemies |= tables.bit[board->eptarget];
    if (board->checkmask & tables.bit[ENPOFF(board)])
        checkmask |= tables.bit[board->eptarget];

    targets = tables.pawn_caps[board->side][i];
    targets &= enemies;
    targets &= checkmask;
    return targets;
}

/**
 * pawntargets
 *  return a bitboard of pawn targets from the given square
 */
U64 pawntargets(const Board *board, const int i)
{
    return pawnquiets(board, i) | pawncaps(board, i);
}

/**
 * pinnedpawn
 *  return a bitboard of targets for a pinned pawn
 */
U64 pinnedpawn(const Board *board, const int i)
{
    if (tables.bit[i] & board->pins[DIAG])
        return pawncaps(board, i) & board->pins[DIAG];
    else if (board->capmask == ~0ull)
        return pawnquiets(board, i) & board->pins[CROSS];
    return 0ull;
}

/**
 * knighttargets
 *  return a bitboard of knight targets from the given square
 */
U64 knighttargets(const Board *board, const int i)
{
    U64 targets;

    targets = tables.knight_moves[i];
    targets &= ~board->colorbb[board->side];
    targets &= board->checkmask;
    return targets;
}

/**
 * pinnedknight
 *  return a bitboard of targets for a pinned knight
 */
U64 pinnedknight(const Board *board, const int i)
{
    return 0;
}

/**
 * bishoptargets
 *  return a bitboard of bishop targets from the given square
 */
U64 bishoptargets(const Board *board, const int i)
{
    U64 ray;

    ray = board_slide045(board, i) | board_slide135(board, i);
    ray &= ~board->colorbb[board->side];
    ray &= board->checkmask;
    return ray;
}

/**
 * pinnedbishop
 *  return a bitboard of targets for a pinned bishop
 */
U64 pinnedbishop(const Board *board, const int i)
{
    if (tables.bit[i] & board->pins[CROSS])
        return 0;
    return bishoptargets(board, i) & board->pins[DIAG];
}

/**
 * rooktargets
 *  return a bitboard of rook targets from the given square
 */
U64 rooktargets(const Board *board, const int i)
{
    U64 ray;

    ray = board_slide000(board, i) | board_slide090(board, i);
    ray &= ~board->colorbb[board->side];
    ray &= board->checkmask;
    return ray;
}

/**
 * pinnedrook
 *  return a bitboard of targets for a pinned rook
 */
U64 pinnedrook(const Board *board, const int i)
{
    if (tables.bit[i] & board->pins[DIAG])
        return 0;
    return rooktargets(board, i) & board->pins[CROSS];
}

/**
 * queentargets
 *  return a bitboard of queen targets from the given square
 */
U64 queentargets(const Board *board, const int i)
{
    return bishoptargets(board, i) | rooktargets(board, i);
}

/**
 * pinnedqueen
 *  return a bitboard of targets for a pinned queen
 */
U64 pinnedqueen(const Board *board, const int i)
{
    return pinnedbishop(board, i) | pinnedrook(board, i);;
}

/**
 * kingtargets
 *  return a bitboard of king targets from the given square
 */
U64 kingtargets(const Board *board, const int i)
{
    U64 targets;
    
    targets = tables.king_moves[i];
    targets &= ~board->colorbb[board->side];
    targets &= ~board->seen;
    return targets;
}

/**
 * eplegal
 *  test if the king is in check after an enpassant capture is make
 */
int eplegal(Board *board, const Move move)
{
    int legal;

    board_make(board, move);
    board->side ^= 1;

    const U64 allyking = board->piecebb[KING] & board->colorbb[board->side];
    const U64 enemies = board->colorbb[board->side ^ 1];
    const U64 pawns = board->piecebb[PAWN] & enemies;
    const U64 bishops = board->piecebb[BISHOP] & enemies;
    const U64 rooks = board->piecebb[ROOK] & enemies;
    const U64 queens = board->piecebb[QUEEN] & enemies;

    const int i = LSB(allyking);
    legal = 1;
    if ((pawncaps(board, i) & pawns) ||
        (bishoptargets(board, i) & (bishops | queens)) ||
        (rooktargets(board, i)) & (rooks | queens))
        legal = 0;

    board->side ^= 1;
    board_unmake(board, move);

    return legal;
}

/**
 * serialize
 *  add all moves for the specified pieces to the move list
 */
void serialize(Board *board, Move *movelist, int *count, const int piece,
               U64 bitboard, U64 (* const moves)(const Board *, const int))
{
    int i, j, cap;
    U64 targets;

    while (bitboard) {
        i = board_pullbit(&bitboard);
        targets = moves(board, i) & board->capmask;
        while (targets) {
            j = board_pullbit(&targets);
            for (cap = EMPTY; cap < KING; ++cap)
                if (board->piecebb[cap] & tables.bit[j])
                    break;
            Move move = FROM_(i) | TO_(j) | ATTACKER_(piece) | TARGET_(cap);
            if (piece == PAWN) {
                if ((j & 56) == 0 || (j & 56) == 56) {
                    move |= FLAGS_(PROMOTION);
                    movelist[(*count)++] = move | PROMO_(QUEEN);
                    movelist[(*count)++] = move | PROMO_(ROOK);
                    movelist[(*count)++] = move | PROMO_(BISHOP);
                } else if (j == board->eptarget) {
                    move |= FLAGS_(ENPASSANT);
                    if (!eplegal(board, move))
                        continue;
                }
            }
            assert(*count < 128);
            movelist[(*count)++] = move;
        }
    }
}

/**
 * movegen
 *  generate legal moves for the specified piece type
 */
void movegen(Board *board, Move *movelist, int *count, const int piece,
             U64 (* const moves)(const Board *, const int),
             U64 (* const pinnedmoves)(const Board *, const int))
{
    U64 piecebb, free, pinned;

    piecebb = board->piecebb[piece] & board->colorbb[board->side];
    free = piecebb & ~(board->pins[CROSS] | board->pins[DIAG]);
    pinned = piecebb & (board->pins[CROSS] | board->pins[DIAG]);
    
    serialize(board, movelist, count, piece, free, moves);
    serialize(board, movelist, count, piece, pinned, pinnedmoves);
}

/**
 * kinggen
 *  generate legal king moves
 */
void kinggen(Board *board, Move *movelist, int *count)
{
    U64 king;

    king = board->piecebb[KING] & board->colorbb[board->side];

    serialize(board, movelist, count, KING, king, kingtargets);
}

/**
 * castlegen
 *  add legal castling moves to the move list
 */
void castlegen(const Board *board, Move *movelist, int *count)
{
    Move move;

    if (RIGHTS_OO(board)) {
        if (OPEN_OO(board) && SAFE_OO(board)) {
            move = board->side == WHITE ? MOVE_WHITE_OO : MOVE_BLACK_OO;
            assert(*count < 128);
            movelist[(*count)++] = move;
        }
    }
    if (RIGHTS_OOO(board)) {
        if (OPEN_OOO(board) && SAFE_OOO(board)) {
            move = board->side == WHITE ? MOVE_WHITE_OOO : MOVE_BLACK_OOO;
            assert(*count < 128);
            movelist[(*count)++] = move;
        }
    }
}

/**
 * board_generate
 *  generate all legal moves in the current position and return the count
 */
int board_generate(Board *board, Move *movelist)
{
    int count;

    count = 0;

    setdanger(board);

    if (board->nchecks < 2) {
        movegen(board, movelist, &count, PAWN, pawntargets, pinnedpawn);
        movegen(board, movelist, &count, KNIGHT, knighttargets, pinnedknight);
        movegen(board, movelist, &count, BISHOP, bishoptargets, pinnedbishop);
        movegen(board, movelist, &count, ROOK, rooktargets, pinnedrook);
        movegen(board, movelist, &count, QUEEN, queentargets, pinnedqueen);
        if (!board->nchecks && CASTLE_RIGHTS(board))
            castlegen(board, movelist, &count);
    }
    kinggen(board, movelist, &count);

    return count;
}

/**
 * board_captures
 *  generate all legal captures in the current position and return the count
 */
int board_captures(Board *board, Move *movelist)
{
    int count;

    count = 0;

    setdanger(board);

    board->capmask = board->colorbb[board->side ^ 1];
    if (board->nchecks < 2) {
        movegen(board, movelist, &count, PAWN, pawncaps, pinnedpawn);
        movegen(board, movelist, &count, KNIGHT, knighttargets, pinnedknight);
        movegen(board, movelist, &count, BISHOP, bishoptargets, pinnedbishop);
        movegen(board, movelist, &count, ROOK, rooktargets, pinnedrook);
        movegen(board, movelist, &count, QUEEN, queentargets, pinnedqueen);
    }
    kinggen(board, movelist, &count);
    board->capmask = ~0ull;

    return count;
}

/**
 * domove
 *  update the board based on the specified move
 */
void domove(Board *board, Move move)
{
    int from, to, attacker, target, flags, promo;

    from = FROM(move);
    to = TO(move);
    attacker = ATTACKER(move);
    target = TARGET(move);
    flags = FLAGS(move);
    promo = PROMO(move);

    board->piecebb[attacker] ^= tables.bit[from] | tables.bit[to];
    board->colorbb[board->side] ^= tables.bit[from] | tables.bit[to];
    if (attacker == PAWN) {
        if ((from ^ to) == 16) {
            U64 enemy_pawns = board->piecebb[PAWN];
            enemy_pawns &= board->colorbb[board->side ^ 1];
            if ((tables.bit[to - 1] & enemy_pawns & ~tables.file_masks[7]) ||
                (tables.bit[to + 1] & enemy_pawns & ~tables.file_masks[0]))
                board->eptarget = from ^ 24;
        }
        if (flags == PROMOTION) {
            board->piecebb[PAWN] ^= tables.bit[to];
            board->piecebb[promo] ^= tables.bit[to];
        } else if (flags == ENPASSANT) {
            target = PAWN;
            to = board->side == WHITE ? to - 8 : to + 8;
        }
        board->rule50 = 0;
    } else if (attacker == ROOK) {
        board->castling &= tables.revoke_castling[from];
    } else if (attacker == KING) {
        board->castling &= tables.revoke_castling[from];
        if (flags == CASTLING) {
            board->piecebb[ROOK] ^= tables.rook_switch[to];
            board->colorbb[board->side] ^= tables.rook_switch[to];
        }
    }
    if (target != EMPTY) {
        board->piecebb[target] ^= tables.bit[to];
        board->colorbb[board->side ^ 1] ^= tables.bit[to];
        if (target == ROOK)
            board->castling &= tables.revoke_castling[to];
    }
    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];
}

/**
 * board_make
 *  play the move, updating all relevant board information
 */
void board_make(Board *board, Move move)
{
    U16 undo;

    undo = RULE50_(board->rule50);
    undo |= ENP_(board->eptarget);
    undo |= CASTLE_(board->castling);
    board->undo[board->plynb] = undo;
    ++board->plynb;
    assert(board->plynb < 1024);
    ++board->rule50;
    board->eptarget = 0;

    domove(board, move);

    board->side ^= 1;
    board->history[board->plynb] = hashposition(board);
}

/**
 * board_unmake
 *  undo the move, resetting all relevant board information
 */
void board_unmake(Board *board, Move move)
{
    board->side ^= 1;

    domove(board, move);

    --board->plynb;
    board->rule50 = RULE50(board->undo[board->plynb]);
    board->eptarget = ENP(board->undo[board->plynb]);
    board->castling = CASTLE(board->undo[board->plynb]);
}

/**
 * isrepitition
 *  return whether the current position is a threefold repitition
 */
int isrepitition(const Board *board)
{
    int count, i, j;

    count = 1;
    for (i = board->plynb - board->rule50; i < board->plynb; ++i) {
        if (board->history[board->plynb] == board->history[i]) {
            ++count;
            if (count == 3)
                return 1;
        }
    }

    return 0;
}

/**
 * board_gameover
 *  return whether the game has reached a terminal state
 */
int board_gameover(const Board *board, const int legal_moves)
{
    if (legal_moves == 0 || board->rule50 == 50 || isrepitition(board))
        return 1;
    return 0;
}

/**
 * board_terminal
 *  return the outcome of the game, assuming the game has reached a terminal
 *  state
 */
int board_terminal(const Board *board, const int legal_moves, int depth)
{
    if (legal_moves == 0 && board->nchecks) {
        return CHECKMATE - depth;
    }
    return 0;
}
