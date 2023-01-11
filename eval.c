#include "eval.h"

enum pstindex { PAWN_PST, KNIGHT_PST, KING_MG_PST, KING_EG_PST };

const int pawn_pst[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    25,  30,  30,  35,  35,  30,  30,  25,
    20,  25,  25,  30,  30,  25,  25,  20,
    15,  20,  20,  25,  25,  20,  20,  15,
    10,  15,  15,   0,   0,  15,  15,  10,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
};

const int knight_pst[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  10,  10,  10,  10,  10,  10,   0,
    0,  10,  20,  20,  20,  20,  10,   0,
    0,  10,  20,  30,  30,  20,  10,   0,
    0,  10,  20,  30,  30,  20,  10,   0,
    0,  10,  20,  20,  20,  20,  10,   0,
    0,  10,  10,  10,  10,  10,  10,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
};

const int king_mg_pst[] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20,
};

const int king_eg_pst[] = {
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
const int *pst[] = { pawn_pst, knight_pst, king_mg_pst, king_eg_pst };

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
 * nbishop_attacks
 *  return the number of bishop attacks
 */
int nbishop_attacks(const Board *board, U64 piecebb, const int side)
{
    int i, ray, count;

    count = 0;
    while (piecebb) {
        i = board_pullbit(&piecebb);
        ray = board_slide045(board, i) | board_slide135(board, i);
        ray &= ~board->colorbb[side];
        count += POPCNT(ray);
    }
    return count;
}

/**
 * nrook_attacks
 *  return the number of rook attacks
 */
int nrook_attacks(const Board *board, U64 piecebb, const int side)
{
    int i, ray, count;

    count = 0;
    while (piecebb) {
        i = board_pullbit(&piecebb);
        ray = board_slide000(board, i) | board_slide090(board, i);
        ray &= ~board->colorbb[side];
        count += POPCNT(ray);
    }
    return count;
}

/**
 * crawlers
 *  return the score for crawling pieces based on material and standing
 */
int crawlers(const Board *board, const int piece, const int pst_idx)
{
    int i;
    int material, standing;
    U64 allies, enemies;

    allies = board->piecebb[piece] & board->colorbb[board->side];
    enemies = board->piecebb[piece] & board->colorbb[board->side ^ 1];

    material = (POPCNT(allies) - POPCNT(enemies)) * values[piece];

    standing = 0;
    if (board->side == WHITE) {
        standing += pstbonus(allies, pst[pst_idx], 56);
        standing -= pstbonus(enemies, pst[pst_idx], 0);
    } else {
        standing += pstbonus(allies, pst[pst_idx], 0);
        standing -= pstbonus(enemies, pst[pst_idx], 56);
    }

    return material + standing;
}

/**
 * sliders
 *  return the score for sliding pieces based on material and mobility
 */
int sliders(const Board *board, const int piece,
            int (* const nattacks)(const Board *, U64, const int))
{
    int material, mobility;
    U64 allies, enemies;

    allies = board->piecebb[piece] & board->colorbb[board->side];
    enemies = board->piecebb[piece] & board->colorbb[board->side ^ 1];

    material = (POPCNT(allies) - POPCNT(enemies)) * values[piece];

    mobility = 0;
    mobility += nattacks(board, allies, board->side);
    mobility -= nattacks(board, enemies, board->side ^ 1);
    mobility *= 64 / POPCNT(board->colorbb[BOTH]);

    return material + mobility;
}

/**
 * queens
 *  return the score for queens based on material and mobility
 */
int queens(const Board *board)
{
    int material, mobility;
    U64 allies, enemies;

    allies = board->piecebb[QUEEN] & board->colorbb[board->side];
    enemies = board->piecebb[QUEEN] & board->colorbb[board->side ^ 1];

    material = (POPCNT(allies) - POPCNT(enemies)) * values[QUEEN];

    mobility = 0;
    mobility += nbishop_attacks(board, allies, board->side);
    mobility += nrook_attacks(board, allies, board->side);
    mobility -= nbishop_attacks(board, enemies, board->side ^ 1);
    mobility -= nrook_attacks(board, enemies, board->side ^ 1);
    mobility *= 32 / POPCNT(board->colorbb[BOTH]);

    return material + mobility;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(const Board *board)
{
    int score;

    score = 0;
    score += crawlers(board, PAWN, PAWN_PST);
    score += crawlers(board, KNIGHT, KNIGHT_PST);
    score += sliders(board, BISHOP, nbishop_attacks);
    score += sliders(board, ROOK, nrook_attacks);
    score += queens(board);
    if (board->piecebb[QUEEN])
        score += crawlers(board, KING, KING_MG_PST);
    else
        score += crawlers(board, KING, KING_EG_PST);
    return score;
}
