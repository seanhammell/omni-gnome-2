#include "eval.h"

const int empty_pst[64] = {0};

const int pawn_pst[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
};

const int knight_pst[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50,
};

const int bishop_pst[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
};

const int rook_pst[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   0,   0,   5,   5,   0,   0,   0,
};

const int queen_pst[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20,
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

const int *piece_square_tables[] = {
    empty_pst, pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst
};

/**
 * material
 *  evaluate the material difference between sides
 */
int material(Board *board, U64 *allies, U64 *enemies)
{
    int i, score;

    const int values[] = {0, 100, 320, 330, 500, 900, 20000};

    score = 0;
    for (i = PAWN; i <= KING; ++i) {
        score += values[i] * (POPCNT(allies[i]) - POPCNT(enemies[i]));
    }
    return score;
}

/**
 * pst_bonus
 *  calculate bonuses for which square a piece is standing on
 */
int pst_bonus(Board *board, U64 allies, U64 enemies, int piece_type)
{
    int i, score;
    int ally_flip, enemy_flip;

    ally_flip = 56 * board->side;
    enemy_flip = 56 * (board-> side ^ 1);

    while (allies) {
        i = board_pullbit(&allies);
        score += piece_square_tables[piece_type][i ^ ally_flip];
    }
    while (enemies) {
        i = board_pullbit(&enemies);
        score -= piece_square_tables[piece_type][i ^ enemy_flip];
    }
    return score;
}

/**
 * king_pst_bonus
 *  calculate the bonus for a king depending on game phase
 */
int king_pst_bonus(Board *board, U64 ally_king, U64 enemy_king)
{
    int i, score;
    int ally_flip, enemy_flip;

    ally_flip = 56 * board->side;
    enemy_flip = 56 * (board-> side ^ 1);

    score = 0;
    if (board->piecebb[QUEEN]) {
        score += king_middle_game_pst[LSB(ally_king) ^ ally_flip];
        score += king_middle_game_pst[LSB(enemy_king) ^ enemy_flip];
    } else {
        score += king_end_game_pst[LSB(ally_king) ^ ally_flip];
        score += king_end_game_pst[LSB(enemy_king) ^ enemy_flip];
    }
    return score;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(Board *board)
{
    int i, score;
    U64 pawns, knights, bishops, rooks, queens, kings;

    pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    knights = board->piecebb[KNIGHT] & board->colorbb[board->side];
    bishops = board->piecebb[BISHOP] & board->colorbb[board->side];
    rooks = board->piecebb[ROOK] & board->colorbb[board->side];
    queens = board->piecebb[QUEEN] & board->colorbb[board->side];

    U64 allies[7] = {0, pawns, knights, bishops, rooks, queens, kings};

    pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];
    knights = board->piecebb[KNIGHT] & board->colorbb[board->side ^ 1];
    bishops = board->piecebb[BISHOP] & board->colorbb[board->side ^ 1];
    rooks = board->piecebb[ROOK] & board->colorbb[board->side ^ 1];
    queens = board->piecebb[QUEEN] & board->colorbb[board->side ^ 1];

    U64 enemies[7] = {0, pawns, knights, bishops, rooks, queens, kings};

    score = 0;
    score += material(board, allies, enemies);
    for (i = PAWN; i < KING; ++i)
        score += pst_bonus(board, allies[i], enemies[i], i);
    score += king_pst_bonus(board, allies[KING], enemies[KING]);
    return score;
}

