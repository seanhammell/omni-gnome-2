#include "eval.h"

static const int values[] = {0, 100, 300, 300, 500, 900};

/**
 * material
 *  return the material score for the given piece type
 */
int material(const Board *board, const int piece_type)
{
    int material;
    U64 allies, enemies;

    allies = board->piecebb[piece_type] & board->colorbb[board->side];
    enemies = board->piecebb[piece_type] & board->colorbb[board->side ^ 1];

    material = POPCNT(allies) - POPCNT(enemies);
    material *= values[piece_type];

    return material;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(const Board *board)
{
    int score;

    score = 0;
    score += material(board, PAWN);
    score += material(board, KNIGHT);
    score += material(board, BISHOP);
    score += material(board, ROOK);
    score += material(board, QUEEN);
    return score;
}
