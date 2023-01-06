#include <stdio.h>

#include "board.h"

/* intrinsics for bitboard serialization */
#define lsb(x)      __builtin_ctzll(x)
#define popcnt(x)   __builtin_popcountll(x)

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

typedef struct tables Tables;

struct tables {
    U64 bit[64];
    U64 rank_masks[64];
    U64 file_masks[64];
    U64 diag_masks[64];
    U64 anti_masks[64];

    U64 knight_moves[64];
    U64 king_moves[64];
    U64 rays[8][256];
};

static Tables tables;

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

/**
 * board_inittables
 *  Call table initialization functions
 */
void board_inittables()
{
    initmasks();
    initknights();
    initkings();
    initrays();
}

/**
 * board_parsefen
 *  initialize the board based on the provided FEN string.
 */
void board_parsefen(Board *board, char *fen)
{
    char c, pos[128], side, castle[5], ep[3];
    const char pchars[] = " PNBRQK??pnbrqk";
    int rule50, movenb;
    int i, rank, file;
    U64 sq;
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
    board->movenb = movenb;
}

/**
 * board_display
 *  Print the current board position.
 */
void board_display(const Board *board)
{
    const char pchars[] = " PNBRQK??pnbrqk";
    int rank, file;
    U64 sq;
    Color color;
    Piece piece;

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
    if (board->eptarget == 64)
        printf("-");
    else
        printf("%d", board->eptarget);
    printf("\nhalfmove clock: %d", board->rule50);
    printf("\nfullmove number: %d\n\n", board->movenb);
}

void board_printmove(MoveInfo *minfo)
{
    int from, to;

    from = from(minfo->move);
    to = to(minfo->move);
    printf("%c%c", 'a' + (from % 8), '1' + (from / 8));
    printf("%c%c", 'a' + (to % 8), '1' + (to / 8));
}

/**
 * pullbit
 *  pop the lsb from the board and return it's index
 */
int pullbit(U64 *bb)
{
    const int i = lsb(*bb);
    *bb &= *bb - 1;
    return i;
}

/**
 * slide000
 *  compute horizontal sliding attacks
 */
U64 slide000(const Board *board, const int i)
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
 * slide045
 *  compute diagonal sliding attacks
 */
U64 slide045(const Board *board, const int i)
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
 * slide090
 *  compute file sliding attacks
 */
U64 slide090(const Board *board, const int i)
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
 * slide135
 *  compute anti-diagonal sliding attacks
 */
U64 slide135(const Board *board, const int i)
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
void pins(Board *board, U64 (*slide)(const Board *, int), const Piece s)
{
    int i;
    U64 sliders, pin;
    U64 enemyray;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[s] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    i = lsb(king);
    const U64 kingray = slide(board, i);
    while(sliders) {
        i = pullbit(&sliders);
        enemyray = slide(board, i);
        pin = kingray & enemyray & board->colorbb[board->side];
        if (pin)
            board->pins[s % 2] |= (slide(board, lsb(pin)) | pin) ^ king;
    }
}

/**
 * seenray
 *  compute the squares seen by the enemy along the given ray
 */
void seenray(Board *board, U64 (*slide)(const Board *, int), const Piece s)
{
    int i;
    U64 sliders;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[s] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    board->colorbb[BOTH] ^= king;
    while(sliders) {
        i = pullbit(&sliders);
        board->seen |= slide(board, i);
    }
    board->colorbb[BOTH] ^= king;
}

/**
 * checkray
 *  compute the check mask along the given ray
 */
void checkray(Board *board, U64 (*slide)(const Board *, int), const Piece s)
{
    int i;
    U64 sliders;
    U64 enemyray;

    const U64 king = board->piecebb[KING] & board->colorbb[board->side];
    sliders = board->piecebb[s] | board->piecebb[QUEEN];
    sliders &= board->colorbb[board->side ^ 1];

    i = lsb(king);
    const U64 kingray = slide(board, i);
    sliders &= kingray;
    while(sliders) {
        i = pullbit(&sliders);
        enemyray = slide(board, i);
        board->checkmask |= (kingray & enemyray) | tables.bit[i];
        ++board->nchecks;
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

    i = lsb(king);
    const U64 checks = tables.knight_moves[i] & knights;
    board->checkmask |= checks;
    board->nchecks += popcnt(checks);
    while (knights) {
        i = pullbit(&knights);
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
    board->seen |= tables.king_moves[lsb(king)];
}

/**
 * setdanger
 *  find pinned pieces, squares seen by the enemy, and checks
 */
void setdanger(Board *board)
{
    int i;

    const Piece sliders[4] = {ROOK, BISHOP, ROOK, BISHOP};
    U64 (* const slide[4])(const Board *, const int) = {
        slide000, slide045, slide090, slide135
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

    knightdanger(board);
    kingdanger(board);

    if (!board->checkmask)
        board->checkmask = ~board->checkmask;
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

    ray = slide045(board, i) | slide135(board, i);
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
    return bishoptargets(board, i) & board->pins[DIAG];
}

/**
 * rooktargets
 *  return a bitboard of rook targets from the given square
 */
U64 rooktargets(const Board *board, const int i)
{
    U64 ray;

    ray = slide000(board, i) | slide090(board, i);
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
    if (board->pins[CROSS] & tables.bit[i])
        return pinnedrook(board, i);
    else
        return pinnedbishop(board, i);
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
 * serialize
 *  add all moves for the specified pieces to the move list
 */
void serialize(const Board *board, MoveInfo *movelist, int *count,
               const Piece piece,
               U64 bitboard, U64 (* const moves)(const Board *, const int))
{
    int i, j, cap;
    U64 targets;

    while (bitboard) {
        i = pullbit(&bitboard);
        targets = moves(board, i);
        while (targets) {
            j = pullbit(&targets);
            MoveInfo minfo;
            minfo.attacker = piece;
            for (cap = EMPTY; cap < KING; ++cap)
                if (board->piecebb[cap] & tables.bit[j])
                    break;
            minfo.target = cap;
            minfo.move = move(i, j, 0, 0);
            movelist[*count] = minfo;
            ++(*count);
        }
    }
}

/**
 * movegen
 *  generate legal moves for the specified piece type
 */
void movegen(const Board *board, MoveInfo *movelist, int *count,
             const Piece piece, U64 (* const moves)(const Board *, const int),
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
void kinggen(const Board *board, MoveInfo *movelist, int *count)
{
    U64 king;

    king = board->piecebb[KING] & board->colorbb[board->side];

    serialize(board, movelist, count, KING, king, kingtargets);
}

/**
 * board_generate
 *  generate all legal moves in the current position and return the count
 */
int board_generate(Board *board, MoveInfo *movelist)
{
    int count;

    count = 0;

    setdanger(board);

    if (board->nchecks < 2) {
        movegen(board, movelist, &count, KNIGHT, knighttargets, pinnedknight);
        movegen(board, movelist, &count, BISHOP, bishoptargets, pinnedbishop);
        movegen(board, movelist, &count, ROOK, rooktargets, pinnedrook);
        movegen(board, movelist, &count, QUEEN, queentargets, pinnedqueen);
    }
    kinggen(board, movelist, &count);

    return count;
}

/**
 * domove
 *  update the bitboards based on the specified move
 */
void domove(Board *board, int from, int to, Piece attacker, Piece target)
{
    board->piecebb[attacker] ^= tables.bit[from] | tables.bit[to];
    board->colorbb[board->side] ^= tables.bit[from] | tables.bit[to];
    if (target != EMPTY) {
        board->piecebb[target] ^= tables.bit[to];
        board->colorbb[board->side ^ 1] ^= tables.bit[to];
    }

    board->colorbb[BOTH] = board->colorbb[WHITE] | board->colorbb[BLACK];
    board->piecebb[EMPTY] = ~board->colorbb[BOTH];
}

/**
 * board_make
 *  Play the move, updating all relevant board information
 */
void board_make(Board *board, MoveInfo *minfo)
{
    int from, to, flags, promo;
    Piece attacker, target;

    from = from(minfo->move);
    to = to(minfo->move);
    flags = flags(minfo->move);
    promo = promo(minfo->move);
    attacker = minfo->attacker;
    target = minfo->target;
    minfo->undo = undo(board->rule50, board->eptarget, board->castling);

    if (board->side == BLACK)
        ++board->movenb;
    ++board->rule50;

    domove(board, from, to, attacker, target);

    board->side ^= 1;
}

/**
 * board_unmake
 *  Undo the move, resetting all relevant board information
 */
void board_unmake(Board *board, MoveInfo *minfo)
{
    int from, to, flags, promo, rule50, eptarget;
    Piece attacker, target;
    Castling castling;

    from = from(minfo->move);
    to = to(minfo->move);
    flags = flags(minfo->move);
    promo = promo(minfo->move);
    attacker = minfo->attacker;
    target = minfo->target;
    rule50 = rule50(minfo->undo);
    eptarget = eptarget(minfo->undo);
    castling = castling(minfo->undo);

    board->side ^= 1;

    if (board->side == BLACK)
        --board->movenb;
    board->rule50 = rule50;

    domove(board, from, to, attacker, target);
}
