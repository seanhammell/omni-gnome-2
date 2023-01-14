#include "eval.h"

static const int values[] = {0, 90, 320, 330, 500, 975};
static const int gamephase_inc[] = {0, 0, 1, 1, 2, 4, 0};

static const int pawn_pst[] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    100, 135,  60,  95,  70, 125,  35, -10,
     -5,   5,  25,  30,  65,  55,  25, -20,
    -15,  15,   5,  20,  25,  10,  15, -25,
    -25,   0,  -5,  10,  15,   5,  10, -25,
    -25,  -5,  -5, -10,   5,   5,  30, -10,
    -35,   0, -20, -25, -15,  25,  40, -20,
      0,   0,   0,   0,   0,   0,   0,   0,
};

static const int knight_pst[] = {
    -165, -90, -35, -50,  60, -100, -15, -105,
     -75, -40,  70,  35,  25,   60,   5,  -15,
     -45,  60,  35,  65,  85,  130,  75,   45,
     -10,  15,  20,  55,  35,   70,  20,   20,
     -15,   5,  15,  15,  30,   20,  20,  -10,
     -25, -10,  10,  10,  20,   15,  25,  -15,
     -30, -55, -10,  -5,   0,   20, -15,  -20,
    -105, -20, -60, -35, -15,  -30, -20,  -25,
};

static const int bishop_pst[] = {
    -30,   5, -80, -35, -25, -40,   5, -10,
    -25,  15, -20, -15,  30,  60,  20, -45,
    -15,  35,  45,  40,  35,  50,  40,   0,
     -5,   5,  20,  50,  35,  35,   5,   0,
     -5,  15,  15,  25,  35,  10,  10,   5,
      0,  15,  15,  15,  15,  25,  20,  10,
      5,  15,  15,   0,   5,  20,  35,   0,
    -35,  -5, -15, -20, -15, -10, -40, -20,
};

static const int rook_pst[] = {
     30,  40,  30,  50, 65, 10,  30,  45,
     25,  30,  60,  60, 80, 65,  25,  45,
     -5,  20,  25,  35, 15, 45,  60,  15,
    -25, -10,   5,  25, 25, 35, -10, -20,
    -35, -25, -10,   0, 10, -5,   5, -25,
    -45, -25, -15, -15,  5,  0,  -5, -35,
    -45, -15, -20, -10,  0, 10,  -5, -70,
    -20, -15,   0,  15, 15,  5, -35, -25,
};

static const int queen_pst[] = {
    -30,   0,  30,  10,  60,  45,  45,  45,
    -25, -40,  -5,   0, -15,  55,  30,  55,
    -15, -15,   5,  10,  30,  55,  45,  55,
    -25, -25, -15, -15,   0,  15,   0,   0,
    -10, -25, -10, -10,   0,  -5,   5,  -5,
    -15,   0, -10,   0,  -5,   0,  15,   5,
    -35, -10,  10,   0,  10,  15,  -5,   0,
      0, -20, -10,  10, -15, -25, -30, -50,
};

static const int king_mg_pst[] = {
    -65,  25,  15, -15, -55, -35,   0,  15,
     30,   0, -20,  -5, -10,  -5, -40, -30,
    -10,  25,   0, -15, -20,   5,  20, -20,
    -15, -20, -10, -25, -30, -25, -15, -35,
    -50,   0, -25, -40, -45, -45, -35, -50,
    -15, -15, -20, -45, -45, -30, -15, -25,
      0,   5, -10, -65, -45, -15,  10,  10,
    -15,  35,  10, -55,  10, -30,  25,  15,
};

static const int king_eg_pst[] = {
    -75, -35, -20, -20, -10,  15,   5, -15,
    -10,  15,  15,  15,  15,  40,  25,  10,
     10,  15,  25,  15,  20,  45,  45,  15,
    -10,  20,  25,  25,  25,  35,  25,   5,
    -20,  -5,  20,  25,  25,  25,  10, -10,
    -20,  -5,  10,  20,  25,  15,   5, -10,
    -25, -10,   5,  15,  15,   5,  -5, -15,
    -55, -35, -21, -10, -30, -15, -25, -45,
};

static int gamephase;

/**
 * material_value
 *  calculate the material difference between sides
 */
int material_value(const Board *board)
{
    int i, m_value;
    U64 allybb, ally_pop;
    U64 enemybb, enemy_pop;

    m_value = gamephase = 0;
    for (i = 1; i < 7; ++i) {
        allybb = board->piecebb[i] & board->colorbb[board->side];
        enemybb = board->piecebb[i] & board->colorbb[board->side ^ 1];
        m_value += POPCNT(allybb) * values[i];
        m_value -= POPCNT(enemybb) * values[i];
        gamephase += POPCNT(board->piecebb[i]) * gamephase_inc[i];
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
    return b_value * 40;
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
    return di_value * 40;
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
    return nm_value * 10;
}

#define MAX(a, b)   (a > b ? a : b)
/**
 * long_diagonal
 *  give a bonus for bishops on long, open diagonals
 */
int long_diagonal(Board *board, U64 allies, U64 enemies)
{
    int i, ld_value;
    U64 m_diag, a_diag;

    ld_value = 0;
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
    return ld_value * 20;
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
    while (allies) {
        i = board_pullbit(&allies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side];
        vert = board_slide090(board, i) & ~board->colorbb[board->side];
        of_value += POPCNT(horiz) * 4 + POPCNT(vert) * 16;
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side ^ 1];
        vert = board_slide090(board, i) & ~board->colorbb[board->side ^ 1];
        of_value -= POPCNT(horiz) * 4 + POPCNT(vert) * 16;
    }
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

    p_value = square_bonus(board, PAWN, pawn_pst);
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

    n_value = square_bonus(board, KNIGHT, knight_pst);
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

    b_value = square_bonus(board, BISHOP, bishop_pst);
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

    r_value = square_bonus(board, ROOK, rook_pst);
    r_value += open_file(board, allies, enemies);
    return r_value;
}

/**
 * queen_score
 *  evaluate queens based on standing
 */
int queen_score(Board *board)
{
    int q_value;
    U64 allies, enemies;

    allies = board->piecebb[QUEEN] & board->colorbb[board->side];
    enemies = board->piecebb[QUEEN] & board->colorbb[board->side ^ 1];

    q_value = square_bonus(board, QUEEN, queen_pst);
    return q_value;
}

/**
 * king_score
 *  evaluate kings based on safety or centralization for the middle and end
 *  game respectively
 */
int king_score(Board *board)
{
    int k_mg_value, k_eg_value;
    int mg_phase, eg_phase;
    U64 allies, enemies;

    allies = board->piecebb[KING] & board->colorbb[board->side];
    enemies = board->piecebb[KING] & board->colorbb[board->side ^ 1];

    k_mg_value = square_bonus(board, KING, king_mg_pst);
    k_eg_value = square_bonus(board, KING, king_eg_pst);
    mg_phase = gamephase > 24 ? 24 : gamephase;
    eg_phase = 24 - mg_phase;
    return (k_mg_value * mg_phase + k_eg_value * eg_phase) / 24;
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
    eval += queen_score(board);
    eval += king_score(board);
    return eval;
}
