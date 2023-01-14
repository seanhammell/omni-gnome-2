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

/**
 * material
 *  return the material score for the given piece type
 */
int material(const Board *board)
{
    int i, j, matval;
    U64 allybb, enemybb;

    matval = 0;
    for (i = 1; i < 7; ++i) {
        allybb = board->piecebb[i] & board->colorbb[board->side];
        enemybb = board->piecebb[i] & board->colorbb[board->side ^ 1];
        matval += POPCNT(allybb) * values[i];
        matval -= POPCNT(enemybb) * values[i];
    }
    return matval;
}

/**
 * standing
 *  score the pieces based on the square they are standing on
 */
int standing(const Board *board, const int piece_type, const int *pst)
{
    int i, ally_flip, enemy_flip;
    int standval;
    U64 allies, enemies;

    allies = board->piecebb[piece_type] & board->colorbb[board->side];
    enemies = board->piecebb[piece_type] & board->colorbb[board->side ^ 1];

    ally_flip = board->side == WHITE ? 56 : 0;
    enemy_flip = board->side == WHITE ? 0 : 56;

    standval = 0;
    while (allies) {
        i = board_pullbit(&allies) ^ ally_flip;
        standval += pst[i];
    }
    while (enemies) {
        i = board_pullbit(&enemies) ^ enemy_flip;
        standval -= pst[i];
    }
    return standval;
}

/**
 * blocked
 *  return the difference of blocked ally pawns vs blocked enemy pawns
 */
int blocked(const Board *board, U64 allies, U64 enemies)
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
    b_value = 0;
    b_value -= POPCNT(allies) - POPCNT(allies_push);
    b_value += POPCNT(enemies) - POPCNT(enemies_push);
    b_value *= 20;
    return b_value;
}

/**
 * doubled_isolated
 *  return the difference of doubled or isolated ally pawns vs enemy pawns
 */
int doubled_isolated(const Board *board, U64 allies, U64 enemies)
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
    di_value *= 20;
    return di_value;
}

/**
 * knight_mobility
 *  score the mobility of each knight
 */
int knight_mobility(const Board *board, U64 allies, U64 enemies)
{
    int i, mobility_value;
    U64 moves;

    mobility_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        moves = tables.knight_moves[i] & ~board->colorbb[board->side];
        mobility_value += POPCNT(moves);
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        moves = tables.knight_moves[i] & ~board->colorbb[board->side ^ 1];
        mobility_value -= POPCNT(moves);
    }
    mobility_value *= 5;
    return mobility_value;
}

/**
 * bishop_mobility
 *  score the mobility of each bishop
 */
int bishop_mobility(Board *board, U64 allies, U64 enemies)
{
    int i, mobility_value;
    U64 m_diag, a_diag;

    mobility_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        m_diag = board_slide045(board, i) & ~board->colorbb[board->side];
        a_diag = board_slide135(board, i) & ~board->colorbb[board->side];
        mobility_value += MAX(POPCNT(m_diag), POPCNT(a_diag));
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        m_diag = board_slide045(board, i) & ~board->colorbb[board->side ^ 1];
        a_diag = board_slide135(board, i) & ~board->colorbb[board->side ^ 1];
        mobility_value -= MAX(POPCNT(m_diag), POPCNT(a_diag));
    }
    mobility_value *= 10;
    return mobility_value;
}

/**
 * rook_mobility
 *  score the mobility of each rook
 */
int rook_mobility(Board *board, U64 allies, U64 enemies)
{
    int i, mobility_value;
    U64 horiz, vert;

    mobility_value = 0;
    while (allies) {
        i = board_pullbit(&allies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side];
        vert = board_slide090(board, i) & ~board->colorbb[board->side];
        mobility_value += POPCNT(horiz) * 2 + POPCNT(vert) * 8;
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        horiz = board_slide000(board, i) & ~board->colorbb[board->side ^ 1];
        vert = board_slide090(board, i) & ~board->colorbb[board->side ^ 1];
        mobility_value -= POPCNT(horiz) * 2 + POPCNT(vert) * 8;
    }
    return mobility_value;
}

/**
 * pawns
 *  evaluate pawns in the current position
 */
int pawns(const Board *board)
{
    int pawn_value;
    U64 allies, enemies;

    allies = board->piecebb[PAWN] & board->colorbb[board->side];
    enemies = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    pawn_value = 0;
    pawn_value += blocked(board, allies, enemies);
    pawn_value += doubled_isolated(board, allies, enemies);
    pawn_value += standing(board, PAWN, advance_pst);
    return pawn_value;
}

/**
 * knights
 *  evaluate knights in the current position
 */
int knights(const Board *board)
{
    int knight_value;
    U64 allies, enemies;

    allies = board->piecebb[KNIGHT] & board->colorbb[board->side];
    enemies = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];

    knight_value = 0;
    knight_value += knight_mobility(board, allies, enemies);
    knight_value += standing(board, KNIGHT, center_pst);
    return knight_value;
}

/**
 * bishops
 *  evaluate bishops in the current position
 */
int bishops(Board *board)
{
    int bishop_value;
    U64 allies, enemies;

    allies = board->piecebb[BISHOP] & board->colorbb[board->side];
    enemies = board->piecebb[BISHOP] & board->colorbb[board->side ^ 1];

    if (POPCNT(allies) > 1)
        bishop_value += 30;
    if (POPCNT(enemies) > 1)
        bishop_value -= 30;

    bishop_value = 0;
    bishop_value += bishop_mobility(board, allies, enemies);
    bishop_value += standing(board, BISHOP, diagonal_pst);
    return bishop_value;
}

/**
 * evaluate rooks in the current position
 */
int rooks(Board *board)
{
    int rook_value;
    U64 allies, enemies;

    allies = board->piecebb[ROOK] & board->colorbb[board->side];
    enemies = board->piecebb[ROOK] & board->colorbb[board->side ^ 1];

    rook_value = 0;
    rook_value += rook_mobility(board, allies, enemies);
    rook_value += standing(board, ROOK, back_rank_pst);
    return rook_value;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(Board *board)
{
    int board_eval;

    board_eval = 0;
    board_eval += material(board);
    board_eval += pawns(board);
    board_eval += knights(board);
    board_eval += bishops(board);
    board_eval += rooks(board);
    return board_eval;
}
