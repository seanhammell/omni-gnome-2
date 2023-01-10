#include "eval.h"

/**
 * material
 *  evaluate the material difference between sides
 */
int material(Board *board, U64 *allies, U64 *enemies)
{
    int i, score;

    const int values[] = {100, 320, 330, 500, 900, 20000};

    score = 0;
    for (i = 0; i < 6; ++i) {
        score += values[i] * (POPCNT(allies[i]) - POPCNT(enemies[i]));
    }
    return score;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(Board *board)
{
    int score;
    U64 pawns, knights, bishops, rooks, queens, kings;

    pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    knights = board->piecebb[KNIGHT] & board->colorbb[board->side];
    bishops = board->piecebb[BISHOP] & board->colorbb[board->side];
    rooks = board->piecebb[ROOK] & board->colorbb[board->side];
    queens = board->piecebb[QUEEN] & board->colorbb[board->side];

    U64 allies[6] = {pawns, knights, bishops, rooks, queens, kings};

    pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];
    knights = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];
    bishops = board->piecebb[BISHOP] & board->colorbb[board->side ^ 1];
    rooks = board->piecebb[ROOK] & board->colorbb[board->side ^ 1];
    queens = board->piecebb[QUEEN] & board->colorbb[board->side ^ 1];

    U64 enemies[6] = {pawns, knights, bishops, rooks, queens, kings};

    score = 0;
    score += material(board, allies, enemies);
    return score;
}

