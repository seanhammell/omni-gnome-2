#include "eval.h"

static const int values[] = {0, 100, 320, 330, 500, 975};
static const int gamephase_inc[] = {0, 0, 1, 1, 2, 4, 0};

static const int pawn_pst[] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     40,  60,  60,  80,  80,  60,  60,  40,
    -10,  20,  30,  40,  40,  30,  20, -10,
    -20,  15,  10,  20,  20,  10,  15, -20,
    -25,   0,   0,  15,  15,   0,  10, -25,
    -10,   0,   0,   0,   0,   0,  30, -10,
      0,   0,   0, -20, -20,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
};

static const int outpost_squares[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

static const int knight_pst[] = {
    -135, -50, -70,   0,   0, -70, -50, -135,
     -45, -15,  65,  30,  30,  65, -15,  -45,
       0,  70,  75,  75,  75,  75,  70,    0,
       0,  20,  45,  45,  45,  45,  20,    0,
     -10,  10,  20,  20,  20,  20,  10,  -10,
     -15,   0,  15,  15,  15,  15,   0,  -15,
     -25, -35,   0,   0,   0,   0, -35,  -25,
     -65, -20, -45, -25, -25, -45, -20,  -65,
};

static const int bishop_pst[] = {
    -20,   5, -60, -30, -30, -60,   5, -20,
    -35,  15,  20,   5,   5,  20,  15, -35,
    -10,  30,  45,  35,  35,  45,  30, -10,
      0,   5,  30,  45,  45,  30,   5,   0,
      0,  10,  15,  30,  30,  15,  10,   0,
      5,  15,  20,  15,  15,  20,  15,   5,
      0,  25,  15,   0,   0,  15,  25,   0,
    -30, -20, -15, -20, -20, -15, -20, -30,
};

static const int rook_pst[] = {
     35,  40,  20, 60, 60, 20,  40,  35,
     35,  30,  65, 70, 70, 65,  30,  35,
      5,  40,  40, 25, 25, 40,  40,   5,
    -20,  -5,   0, 25, 25, 35,  -5, -20,
    -30, -10,   0,  5,  5, -5, -10, -30,
    -40, -20, -10, -5, -5,  0, -20, -40,
    -60, -15, -10, -5, -5, 10, -15, -60,
    -25, -25,   0, 15, 15,  0, -25, -25,
};

static const int queen_pst[] = {
      0,  20,  40,  35,  35,  40,  20,   0,
     15,   0,  25,  -5,  -5,  25,   0,  15,
     20,  15,  30,  20,  20,  30,  15,  20,
    -15, -10,   0, -10, -10,   0, -10, -15,
     -5, -10, -10,  -5,  -5, -10, -10,  -5,
     -5,   5,  -5,   0,   0,  -5,   5,  -5,
    -15, -10,  15,   5,   5,  15, -10, -15,
    -25, -30, -15,   0,   0, -15, -30, -25,
};

static const int king_mg_pst[] = {
    -65,  20,   5, -25, -55, -25,   5,  20,
     30, -15, -30,  -5, -10,  -5, -30, -35,
    -10,   0,  10,  -5, -20,  -5,  10,   0,
    -15, -30, -15, -25, -30, -25, -15, -30,
    -50, -25, -30, -40, -45, -40, -30, -25,
    -15, -20, -20, -40, -45, -40, -20, -20,
      0,   5,   0, -45, -45, -45,   0,   5,
    -15,  25,  20, -45,  10, -45,  20,  25,
};

static const int king_eg_pst[] = {
    -45, -20,   0, -20, -20,   0, -20, -45,
      0,  30,  30,  15,  15,  30,  30,   0,
     10,  35,  35,  20,  20,  35,  35,  10,
      5,  20,  20,  25,  25,  20,  20,   5,
    -15,   0,  20,  25,  25,  20,   0, -15,
    -25,   0,  15,  20,  20,  15,   0, -15,
    -20, -10,   5,  15,  15,   5, -10, -20,
    -50, -30, -15, -20, -20, -15, -20, -50,
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
 * knight_outpost
 *  give a bonus for protected knights on outpost squares
 */
int knight_outpost(const Board *board, U64 allies, U64 enemies)
{
    int i, no_value;
    int ally_flip, enemy_flip;
    U64 ally_pawns, enemy_pawns;
    U64 defenders, attackers;

    ally_pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    enemy_pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    ally_flip = board->side == WHITE ? 56 : 0;
    enemy_flip = board->side == WHITE ? 0 : 56;

    no_value = 0;
    while (allies) {
        i = board_pullbit(&allies) ^ ally_flip;
        if (!outpost_squares[i])
            break;
        defenders = ally_pawns & tables.pawn_caps[board->side ^ 1][i];
        attackers = enemy_pawns & tables.pawn_caps[board->side][i];
        if (defenders && !attackers)
            ++no_value;
    }
    while (enemies) {
        i = board_pullbit(&enemies) ^ enemy_flip;
        if (!outpost_squares[i])
            break;
        defenders = enemy_pawns & tables.pawn_caps[board->side][i];
        attackers = ally_pawns & tables.pawn_caps[board->side ^ 1][i];
        if (defenders && !attackers)
            --no_value;
    }
    return no_value * 80;
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
 *  give a bonus for rooks on open files
 */
int open_file(Board *board, U64 allies, U64 enemies)
{
    int i, of_value;
    U64 vert;

    of_value = 0;
    board->colorbb[BOTH] ^= board->piecebb[ROOK];
    while (allies) {
        i = board_pullbit(&allies);
        vert = board_slide090(board, i) & ~board->colorbb[board->side];
        of_value += POPCNT(vert);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        vert = board_slide090(board, i) & ~board->colorbb[board->side ^ 1];
        of_value -= POPCNT(vert);
    }
    board->colorbb[BOTH] ^= board->piecebb[ROOK];
    return of_value * 20;
}

/**
 * king_aggression
 *  give a bonus for an aggressive king as the game progresses
 */
int king_aggression(const Board *board, U64 allies, U64 enemies)
{
    int i, ka_value;
    U64 moves;

    ka_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        moves = tables.king_moves[i] & board->colorbb[board->side ^ 1];
        ka_value += POPCNT(moves);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        moves = tables.king_moves[i] & board->colorbb[board->side];
        ka_value -= POPCNT(moves);
    }
    return ka_value * 80;
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
    n_value += knight_outpost(board, allies, enemies);
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
    k_eg_value += king_aggression(board, allies, enemies);
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
