#include "eval.h"

const int empty_pst[64] = {0};

const int pawn_pst[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    30,  30,  30,  30,  30,  30,  30,  30,
    15,  20,  20,  25,  25,  20,  20,  15,
    10,  15,  15,  20,  20,  15,  15,  10,
     5,  10,  10,  15,  15,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
};

const int center_pst[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  10,  10,  10,  10,  10,  10,   0,
    0,  10,  20,  20,  20,  20,  10,   0,
    0,  10,  20,  30,  30,  20,  10,   0,
    0,  10,  20,  30,  30,  20,  10,   0,
    0,  10,  20,  20,  20,  20,  10,   0,
    0,  10,  10,  10,  10,  10,  10,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
};

const int king_middle_game_pst[] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20,
};

const int king_end_game_pst[] = {
    -50, -40, -30, -20, -20, -30, -40, -50,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -50, -30, -30, -30, -30, -30, -30, -50,
};

const int values[] = {0, 100, 320, 330, 500, 900, 20000};

/**
 * material
 *  evaluate the material difference between sides
 */
int material(Board *board)
{
    int i, score;
    U64 allies, enemies;

    allies = board->colorbb[board->side];
    enemies = board->colorbb[board->side ^ 1];

    score = 0;
    for (i = ROOK; i <= KING; ++i) {
        score += values[i] * POPCNT(board->piecebb[i] & allies);
        score -= values[i] * POPCNT(board->piecebb[i] & enemies);
    }
    return score;
}

/**
 * pstbonus
 *  calculate the piece-square table bonus for the given pieces
 */
int pstbonus(U64 pieces, const int *pst, const int flip)
{
    int i, score;

    score = 0;
    while (pieces) {
        i = board_pullbit(&pieces) ^ flip;
        score += pst[i];
    }
    return score;
}

/**
 * pawns
 *  return the score for pawns based on material, standing, and structure
 */
int pawns(const Board *board)
{
    int i, material, standing, structure;
    int blocked, doubled, isolated;
    U64 ally_pawns, enemy_pawns;
    U64 ally_push, enemy_push;

    blocked = doubled = isolated = 0;

    ally_pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    enemy_pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    material = (POPCNT(ally_pawns) - POPCNT(enemy_pawns)) * values[PAWN];

    for (i = 0; i < 8; ++i) {
        if (POPCNT(ally_pawns & tables.file_masks[i]) > 1)
            --doubled;
        if (POPCNT(enemy_pawns & tables.file_masks[i]) > 1)
            ++doubled;

        if (i > 0) {
            if (!(ally_pawns & tables.file_masks[i - 1]))
                --isolated;
            if (!(enemy_pawns & tables.file_masks[i - 1]))
                ++isolated;
        }
        if (i < 7) {
            if (!(ally_pawns & tables.file_masks[i + 1]))
                --isolated;
            if (!(enemy_pawns & tables.file_masks[i + 1]))
                ++isolated;
        }
    }

    standing = 0;
    if (board->side == WHITE) {
        ally_push = ally_pawns << 8 & ~board->colorbb[BLACK];
        enemy_push = enemy_pawns >> 8 & ~board->colorbb[WHITE];
        standing += pstbonus(ally_pawns, pawn_pst, 0);
        standing -= pstbonus(enemy_pawns, pawn_pst, 56);
    } else {
        ally_push = ally_pawns >> 8 & ~board->colorbb[WHITE];
        enemy_push = enemy_pawns << 8 & ~board->colorbb[BLACK];
        standing += pstbonus(ally_pawns, pawn_pst, 56);
        standing -= pstbonus(enemy_pawns, pawn_pst, 0);
    }
    blocked -= POPCNT(ally_pawns) - POPCNT(ally_push);
    blocked += POPCNT(enemy_pawns) - POPCNT(enemy_push);
    structure = (blocked + doubled + isolated) * 50;

    return material + standing + structure;
}

/**
 * minors
 *  return the score for minor piece material and standing
 */
int minors(const Board *board)
{
    int material, standing;
    U64 ally_knights, ally_bishops;
    U64 enemy_knights, enemy_bishops;

    ally_knights = board->piecebb[KNIGHT] & board->colorbb[board->side];
    ally_bishops = board->piecebb[BISHOP] & board->colorbb[board->side];
    enemy_knights = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];
    enemy_bishops = board->piecebb[BISHOP] & board->colorbb[board->side ^ 1];

    material = 0;
    material += (POPCNT(ally_knights) - POPCNT(enemy_knights)) * values[KNIGHT];
    material += (POPCNT(ally_bishops) - POPCNT(enemy_bishops)) * values[BISHOP];

    standing = 0;
    if (board->side == WHITE) {
        standing += pstbonus(ally_knights, center_pst, 0);
        standing += pstbonus(ally_bishops, center_pst, 0);
        standing -= pstbonus(enemy_knights, center_pst, 56);
        standing -= pstbonus(enemy_bishops, center_pst, 56);
    } else {
        standing += pstbonus(ally_knights, center_pst, 56);
        standing += pstbonus(ally_bishops, center_pst, 56);
        standing -= pstbonus(enemy_knights, center_pst, 0);
        standing -= pstbonus(enemy_bishops, center_pst, 0);
    }

    return material + standing;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(Board *board)
{
    int score;

    score = 0;
    score += material(board);
    score += pawns(board);
    score += minors(board);
    return score;
}
