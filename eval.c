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

const int knight_pst[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40,   0,   0,   0,   0,   0,   0, -40,
    -30,   0,  10,  10,  10,  10,   0, -30,
    -30,   0,  10,  20,  20,  10,   0, -30,
    -30,   0,  10,  20,  20,  10,   0, -30,
    -30,   0,  10,  10,  10,  10,   0, -30,
    -40,   0,   0,   0,   0,   0,   0, -40,
    -50, -40, -30, -30, -30, -30, -40, -50,
};

const int bishop_pst[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,  10,  15,  15,  10,   5, -10,
    -10,  10,  15,  15,  15,  15,  10, -10,
    -10,  15,  15,  15,  15,  15,  15, -10,
    -10,   5,   5,   5,   5,   5,   5, -10,
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
int material(Board *board)
{
    int i, score;
    U64 allies, enemies;

    const int values[] = {0, 100, 320, 330, 500, 900, 20000};

    allies = board->colorbb[board->side];
    enemies = board->colorbb[board->side ^ 1];

    score = 0;
    for (i = PAWN; i <= KING; ++i) {
        score += values[i] * POPCNT(board->piecebb[i] & allies);
        score -= values[i] * POPCNT(board->piecebb[i] & enemies);
    }
    return score;
}

/**
 * structure
 *  evaluate the pawn structure of each side
 */
int structure(Board *board)
{
    int i, score;
    int blocked, doubled, isolated;
    U64 ally_pawns, enemy_pawns;
    U64 ally_push, enemy_push;

    score = blocked = doubled = isolated = 0;

    ally_pawns = board->piecebb[PAWN] & board->colorbb[board->side];
    enemy_pawns = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    for (i = 0; i < 8; ++i) {
        if (POPCNT(ally_pawns & tables.file_masks[i]) > 1)
            --doubled;
        if (POPCNT(enemy_pawns & tables.file_masks[i]) > 1)
            ++doubled;

        if (i > 0)
            if (!(ally_pawns & tables.file_masks[i - 1]))
                --isolated;
            if (!(enemy_pawns & tables.file_masks[i - 1]))
                ++isolated;
        if (i < 7)
            if (!(ally_pawns & tables.file_masks[i + 1]))
                --isolated;
            if (!(enemy_pawns & tables.file_masks[i + 1]))
                ++isolated;
    }

    if (board->side == WHITE) {
        ally_push = ally_pawns << 8 & ~board->colorbb[BLACK];
        enemy_push = enemy_pawns >> 8 & ~board->colorbb[WHITE];
    } else {
        ally_push = ally_pawns >> 8 & ~board->colorbb[WHITE];
        enemy_push = enemy_pawns << 8 & ~board->colorbb[BLACK];
    }

    blocked -= POPCNT(ally_pawns) - POPCNT(ally_push);
    blocked += POPCNT(enemy_pawns) - POPCNT(enemy_push);

    score = blocked + doubled + isolated;
    score *= 50;
    return score;
}

/**
 * pst_bonus
 *  calculate bonuses based on which square a piece is standing on
 */
int pst_bonus(Board *board)
{
    int i, j, score;
    int ally_sq_flip, enemy_sq_flip;
    U64 allies, enemies, piece, ally_king, enemy_king;

    allies = board->colorbb[board->side];
    enemies = board->colorbb[board->side ^ 1];

    ally_sq_flip = 56 * board->side;
    enemy_sq_flip = 56 * (board->side ^ 1);

    score = 0;
    for (i = PAWN; i < KING; ++i) {
        piece = board->piecebb[i] & allies;
        while (piece) {
            j = board_pullbit(&piece) ^ ally_sq_flip;
            score += piece_square_tables[i][j];
        }
        piece = board->piecebb[i] & enemies;
        while (piece) {
            j = board_pullbit(&piece) ^ enemy_sq_flip;
            score -= piece_square_tables[i][j];
        }
    }
    ally_king = board->piecebb[KING] & allies;
    enemy_king = board->piecebb[KING] & enemies;
    if (board->piecebb[QUEEN]) {
        score += king_middle_game_pst[LSB(ally_king) ^ ally_sq_flip];
        score -= king_middle_game_pst[LSB(enemy_king) ^ enemy_sq_flip];
    } else {
        score += king_end_game_pst[LSB(ally_king) ^ ally_sq_flip];
        score -= king_end_game_pst[LSB(enemy_king) ^ enemy_sq_flip];
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

    score = 0;
    score += material(board);
    score += structure(board);
    score += pst_bonus(board);
    return score;
}
