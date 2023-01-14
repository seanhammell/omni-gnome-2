#include "eval.h"

#define MAX(a, b)   (a > b ? a : b)

static const int values[] = {0, 100, 320, 330, 500, 900};

static const int advance_pst[] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    20,  25,  25,  30,  30,  25,  25,  20,
    10,  15,  15,  25,  25,  15,  15,  10,
     0,   0,   0,  20,  20,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
};

static const int center_pst[] = {
    -40, -30, -20, -20, -20, -20, -30, -40,
    -30, -20,   0,   0,   0,   0, -20, -30,
    -20,   0,  10,  15,  15,  10,   0, -20,
    -20,   0,  15,  20,  20,  15,   0, -20,
    -20,   0,  15,  20,  20,  15,   0, -20,
    -20,   0,  10,  15,  15,  10,   0, -20,
    -30, -20,   0,   0,   0,   0, -20, -30,
    -40, -30, -20, -20, -20, -20, -30, -40,
};

static const int diagonal_pst[] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   5,  10,  10,  10,  10,   5, -10,
    -10,  10,  10,  15,  15,  10,  10, -10,
    -10,  10,   0,   0,   0,   0,  10, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
};

static const int back_rank_pst[] = {
     0,  10,  10,  10,  10,  10,  10,   0,
    10,  20,  20,  20,  20,  20,  20,  10,
     0,  10,  10,  10,  10,  10,  10,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
};

static const int king_safety_pst[] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20,
};

/**
 * material_value
 *  calculate the material difference between sides
 */
int material_value(const Board *board)
{
    int i, m_value;
    U64 allybb, enemybb;

    m_value = 0;
    for (i = 1; i < 7; ++i) {
        allybb = board->piecebb[i] & board->colorbb[board->side];
        enemybb = board->piecebb[i] & board->colorbb[board->side ^ 1];
        m_value += POPCNT(allybb) * values[i];
        m_value -= POPCNT(enemybb) * values[i];
    }
    return m_value;
}

/**
 * square_bonus
 *  give bonuses to pieces depending on which square they are standing on
 */
int square_bonus(const Board *board, const int piece_type, const int *pst)
{
    int i, ally_flip, enemy_flip;
    int sq_bonus;
    U64 allies, enemies;

    allies = board->piecebb[piece_type] & board->colorbb[board->side];
    enemies = board->piecebb[piece_type] & board->colorbb[board->side ^ 1];

    ally_flip = board->side == WHITE ? 56 : 0;
    enemy_flip = board->side == WHITE ? 0 : 56;

    sq_bonus = 0;
    while (allies) {
        i = board_pullbit(&allies) ^ ally_flip;
        sq_bonus += pst[i];
    }
    while (enemies) {
        i = board_pullbit(&enemies) ^ enemy_flip;
        sq_bonus -= pst[i];
    }
    return sq_bonus;
}

/**
 * blocked_pawns
 *  give penalties to each side for blocked pawns
 */
int blocked_pawns(const Board *board, U64 allies, U64 enemies)
{
    int b_value;
    U64 allies_push, enemies_push;

    if (board->side == WHITE) {
        allies_push = (allies << 8) & ~board->colorbb[BOTH];
        enemies_push = (enemies >> 8) & ~board->colorbb[BOTH];
    } else {
        allies_push = (allies >> 8) & ~board->colorbb[BOTH];
        enemies_push = (enemies << 8) & ~board->colorbb[BOTH];
    }
    b_value = POPCNT(allies) - POPCNT(allies_push);
    b_value += POPCNT(enemies) - POPCNT(enemies_push);
    return b_value * 20;
}

/**
 * doubled_isolated_pawns
 *  give penalties to each side for doubled or isolated pawns
 */
int doubled_isolated_pawns(const Board *board, U64 allies, U64 enemies)
{
    int i;
    int file_allies, file_enemies;
    int di_value;
    U64 file, left, right;

    di_value = 0;
    for (i = 0; i < 8; ++i) {
        file = tables.file_masks[i];
        file_allies = POPCNT(allies & file);
        file_enemies = POPCNT(enemies & file);
        if (file_allies > 1)
            di_value -= file_allies;
        if (file_enemies > 1)
            di_value += file_enemies;

        left = i > 0 ? tables.file_masks[i - 1] : 0;
        right = i < 7 ? tables.file_masks[i + 1] : 0;
        if (!(allies & (left | right)))
            di_value -= file_allies;
        if (!(enemies & (left | right)))
            di_value += file_enemies;
    }
    return di_value * 20;
}

/**
 * knight_mobility
 *  calculate the weighted mobility of knights on the board
 */
int knight_mobility(const Board *board, U64 allies, U64 enemies)
{
    int i, nm_value;
    U64 moves;

    nm_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        moves = tables.knight_moves[i] & ~board->colorbb[board->side];
        nm_value += POPCNT(moves);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        moves = tables.knight_moves[i] & ~board->colorbb[board->side ^ 1];
        nm_value -= POPCNT(moves);
    }
    return nm_value * 5;
}

/**
 * long_diagonal
 *  give a bonus for bishops on long, open diagonals
 */
int long_diagonal(Board *board, U64 allies, U64 enemies)
{
    int i, ld_value;
    U64 m_diag, a_diag;

    ld_value = 0;
    board->colorbb[BOTH] ^= board->piecebb[PAWN];
    while (allies) {
        i = board_pullbit(&allies);
        m_diag = board_slide045(board, i) & ~board->colorbb[board->side];
        a_diag = board_slide135(board, i) & ~board->colorbb[board->side];
        ld_value += MAX(POPCNT(m_diag), POPCNT(a_diag));
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        m_diag = board_slide045(board, i) & ~board->colorbb[board->side ^ 1];
        a_diag = board_slide135(board, i) & ~board->colorbb[board->side ^ 1];
        ld_value -= MAX(POPCNT(m_diag), POPCNT(a_diag));
    }
    board->colorbb[BOTH] ^= board->piecebb[PAWN];
    return ld_value * 10;
}

/**
 * open_file
 *  give a bonus for rook mobility with a preference towards open files
 */
int open_file(Board *board, U64 allies, U64 enemies)
{
    int i, of_value;
    U64 horiz, vert;

    of_value = 0;
    board->colorbb[BOTH] ^= board->piecebb[PAWN];
    while (allies) {
        i = board_pullbit(&allies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side];
        vert = board_slide090(board, i) & ~board->colorbb[board->side];
        of_value += POPCNT(horiz) * 2 + POPCNT(vert) * 8;
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side ^ 1];
        vert = board_slide090(board, i) & ~board->colorbb[board->side ^ 1];
        of_value -= POPCNT(horiz) * 2 + POPCNT(vert) * 8;
    }
    board->colorbb[BOTH] ^= board->piecebb[PAWN];
    return of_value;
}

/**
 * pawn_score
 *  evaluate pawns based on standing and structure
 */
int pawn_score(const Board *board)
{
    int p_value;
    U64 allies, enemies;

    allies = board->piecebb[PAWN] & board->colorbb[board->side];
    enemies = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    p_value = square_bonus(board, PAWN, advance_pst);
    p_value += blocked_pawns(board, allies, enemies);
    p_value += doubled_isolated_pawns(board, allies, enemies);
    return p_value;
}

/**
 * knight_score
 *  evaluate knights based on standing and mobility
 */
int knight_score(const Board *board)
{
    int n_value;
    U64 allies, enemies;

    allies = board->piecebb[KNIGHT] & board->colorbb[board->side];
    enemies = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];

    n_value = square_bonus(board, KNIGHT, center_pst);
    n_value += knight_mobility(board, allies, enemies);
    return n_value;
}

/**
 * bishop_score
 *  evaluate bishops based on standing and diagonal control
 */
int bishop_score(Board *board)
{
    int b_value;
    U64 allies, enemies;

    allies = board->piecebb[BISHOP] & board->colorbb[board->side];
    enemies = board->piecebb[BISHOP] & board->colorbb[board->side ^ 1];

    if (POPCNT(allies) > 1)
        b_value += 30;
    if (POPCNT(enemies) > 1)
        b_value -= 30;

    b_value = square_bonus(board, BISHOP, diagonal_pst);
    b_value += long_diagonal(board, allies, enemies);
    return b_value;
}

/**
 * rook_score
 *  evaluate rooks based on standing and file control
 */
int rook_score(Board *board)
{
    int r_value;
    U64 allies, enemies;

    allies = board->piecebb[ROOK] & board->colorbb[board->side];
    enemies = board->piecebb[ROOK] & board->colorbb[board->side ^ 1];

    r_value = square_bonus(board, ROOK, back_rank_pst);
    r_value += open_file(board, allies, enemies);
    return r_value;
}

/**
 * king_score
 *  evaluate kings based on safety or centralization for the middle and end
 *  game respectively
 */
int king_score(Board *board)
{
    int k_value;
    U64 allies, enemies;

    allies = board->piecebb[KING] & board->colorbb[board->side];
    enemies = board->piecebb[KING] & board->colorbb[board->side ^ 1];

    k_value = 0;
    if (board->piecebb[QUEEN])
        k_value += square_bonus(board, KING, king_safety_pst);
    else
        k_value += square_bonus(board, KING, center_pst);
    return k_value;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(Board *board)
{
    int eval;

    eval = material_value(board);
    eval += pawn_score(board);
    eval += knight_score(board);
    eval += bishop_score(board);
    eval += rook_score(board);
    eval += king_score(board);
    return eval;
}
