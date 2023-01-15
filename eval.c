#include "eval.h"

#define CROSS(b, i) (board_slide000(b, i) | board_slide090(b, i))
#define DIAG(b, i) (board_slide045(b, i) | board_slide135(b, i))

static const int values[] = {0, 100, 325, 325, 500, 975};
static const int gamephase_inc[] = {0, 0, 1, 1, 2, 4, 0};

static const int pawn_pst[] = {
     0,  0,  0,   0,   0,  0,  0,  0,
    30, 30, 30,  30,  30, 30, 30, 30,
    10, 10, 15,  20,  20, 15, 10, 10,
     5,  5, 10,  15,  15, 10,  5,  5,
     0,  0,  0,  15,  15,  0,  0,  0,
     0,  0,  0,   0,   0,  0,  0,  0,
     5,  5,  5, -10, -10,  5,  5,  5,
     0,  0,  0,   0,   0,  0,  0,  0,
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
    -25, -20, -15, -15, -15, -15, -20,  -25,
    -20, -10,   0,   0,   0,   0, -10,  -15,
    -15,   0,  20,  30,  30,  20,   0,  -15,
    -15,   0,  30,  40,  40,  30,   0,  -15,
    -15,   0,  30,  40,  40,  30,   0,  -15,
    -15,   0,  20,  30,  30,  20,   0,  -15,
    -20, -10,   0,   5,   5,   0, -10,  -20,
    -25, -20, -15, -15, -15, -15, -20,  -25,
};

static const int bishop_pst[] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
};

static const int rook_pst[] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
     5,  0,  0,  0,  0,  0,  0,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};

static const int queen_pst[] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
      0,  10,  10, 10, 10,  10,  10,   0,
    -10,   0,   5,  5,  5,   5,   0, -10,
     -5,   0,   5,  5,  5,   5,   0,  -5,
     -5,   0,   5,  5,  5,   5,   0,  -5,
    -10,   5,   5,  5,  5,   5,   0, -10,
    -10,   0,   5,  0,  0,   0,   0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20,
};

static const int king_mg_pst[] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     10,  10,   0,   0,   0,   0,  10,  10,
     10,  30,   5,   0,   0,   5,  30,  10,
};

static const int king_eg_pst[] = {
    -25, -20, -15, -15, -15, -15, -20, -25,
    -20, -10,   0,   0,   0,   0, -10, -15,
    -15,   0,  10,  15,  15,  10,   0, -15,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -15,   0,  10,  15,  15,  10,   0, -15,
    -20, -10,   0,   5,   5,   0, -10, -20,
    -25, -20, -15, -15, -15, -15, -20, -25,
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
    return b_value * 10;
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
    return di_value * 10;
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
        i = board_pullbit(&allies);
        if (!outpost_squares[i ^ ally_flip])
            break;
        defenders = ally_pawns & tables.pawn_caps[board->side ^ 1][i];
        attackers = enemy_pawns & tables.pawn_caps[board->side][i];
        if (defenders && !attackers)
            ++no_value;
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        if (!outpost_squares[i ^ enemy_flip])
            break;
        defenders = enemy_pawns & tables.pawn_caps[board->side][i];
        attackers = ally_pawns & tables.pawn_caps[board->side ^ 1][i];
        if (defenders && !attackers)
            --no_value;
    }
    return no_value * 10;
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
    return ld_value * 10;
}

/**
 * open_file
 *  give a bonus for rooks on open files
 */
int open_file(Board *board, U64 allies, U64 enemies)
{
    int i, of_value;
    U64 vert;
    U64 ally_pawns, enemy_pawns;

    ally_pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    enemy_pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    of_value = 0;
    board->colorbb[BOTH] ^= enemy_pawns;
    while (allies) {
        i = board_pullbit(&allies);
        vert = board_slide090(board, i) & ~board->colorbb[board->side];
        of_value += POPCNT(vert);
    }
    board->colorbb[BOTH] ^= ally_pawns | enemy_pawns;
    while (enemies) {
        i = board_pullbit(&enemies);
        vert = board_slide090(board, i) & ~board->colorbb[board->side ^ 1];
        of_value -= POPCNT(vert);
    }
    board->colorbb[BOTH] ^= ally_pawns;
    return of_value * 10;
}

/**
 * queen_threats
 *  give a bonus based on the number of pieces a queen is threatening
 */
int queen_threats(const Board *board, U64 allies, U64 enemies)
{
    int i, qt_value;
    U64 threats;
    U64 ally_targets, enemy_targets;

    ally_targets = board->colorbb[board->side] & ~board->piecebb[QUEEN];
    enemy_targets = board->colorbb[board->side ^ 1] & ~board->piecebb[QUEEN];

    qt_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        threats = CROSS(board, i) | DIAG(board, i);
        threats &= enemy_targets;
        qt_value += POPCNT(threats);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        threats = CROSS(board, i) | DIAG(board, i);
        threats &= ally_targets;
        qt_value -= POPCNT(threats);
    }
    return qt_value * 10;
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
        moves = tables.king_moves[i] & ~board->colorbb[board->side];
        ka_value += POPCNT(moves);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        moves = tables.king_moves[i] & ~board->colorbb[board->side ^ 1];
        ka_value -= POPCNT(moves);
    }
    return ka_value * 10;
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
    if (POPCNT(allies) > 1)
        b_value += 40;
    if (POPCNT(enemies) > 1)
        b_value -= 40;
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
    q_value += queen_threats(board, allies, enemies);
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
