#include "eval.h"

enum pstindex {
    PAWN_PST,
    KNIGHT_PST,
    BISHOP_PST,
    ROOK_PST,
    QUEEN_PST,
    KING_MG_PST,
    KING_EG_PST
};

const int pawn_pst[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     40,  40,  40,  40,  40,  40,  40,  40,
     30,  30,  30,  30,  30,  30,  30,  30,
     20,  20,  20,  20,  20,  20,  20,  20,
     10,  10,  10,  10,  10,  10,  10,  10,
    -10, -10, -15, -20, -20, -15, -10, -10,
      0,   0,   0,   0,   0,   0,   0,   0,
};

const int knight_pst[64] = {
    -15, -15, -15, -15, -15, -15, -15, -15,
    -15,  10,  10,  10,  10,  10,  10, -15,
    -15,  10,  20,  20,  20,  20,  10, -15,
    -15,  10,  20,  30,  30,  20,  10, -15,
    -15,  10,  20,  30,  30,  20,  10, -15,
    -15,  10,  20,  20,  20,  20,  10, -15,
    -15,  10,  10,  10,  10,  10,  10, -15,
    -15, -15, -15, -15, -15, -15, -15, -15,
};

const int bishop_pst[64] = {
    -15, -15, -15, -15, -15, -15, -15, -15,
    -15,   5,   5,   5,   5,   5,   5, -15,
    -15,   5,  10,  20,  20,  10,   5, -15,
    -15,   5,  10,  20,  20,  10,   5, -15,
    -15,   5,  20,  20,  20,  20,   5, -15,
    -15,  20,  20,  20,  20,  20,  20, -15,
    -15,   5,   5,   5,   5,   5,   5, -15,
    -15, -15, -15, -15, -15, -15, -15, -15,
};

const int rook_pst[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     10,  20,  20,  20,  20,  20,  20,  10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
      0,   0,   0,  10,  10,   0,   0,   0,
};

const int queen_pst[64] = {
    -10,  -5,   0,   0,   0,   0,  -5, -10,
     -5,   5,   5,   5,   5,   5,   5,  -5,
      0,   5,  10,  10,  10,  10,   5,   0,
      0,   5,  10,  15,  15,  10,   5,   0,
      0,   5,  10,  15,  15,  10,   5,   0,
      0,   5,  10,  10,  10,  10,   5,   0,
     -5,   5,   5,   5,   5,   5,   5,  -5,
    -10,  -5,   0,   0,   0,   0,  -5, -10,
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
const int *pst[] = {
    pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst, king_mg_pst,
    king_eg_pst
};

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
 * scorepiece
 *  return the score for the given piece type based on material and standing
 */
int scorepiece(const Board *board, const int piece, const int pst_idx)
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
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(const Board *board)
{
    int score;

    score = 0;
    score += scorepiece(board, PAWN, PAWN_PST);
    score += scorepiece(board, KNIGHT, KNIGHT_PST);
    score += scorepiece(board, BISHOP, BISHOP_PST);
    score += scorepiece(board, ROOK, ROOK_PST);
    score += scorepiece(board, QUEEN, QUEEN_PST);
    if (board->piecebb[QUEEN])
        score += scorepiece(board, KING, KING_MG_PST);
    else
        score += scorepiece(board, KING, KING_EG_PST);
    return score;
}
