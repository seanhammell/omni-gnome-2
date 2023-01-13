#include "eval.h"

static const int values[] = {0, 100, 300, 300, 500, 900};

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
U64 blocked(const Board *board, U64 allies, U64 enemies)
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
    return b_value;
}

/**
 * doubled_isolated
 *  return the difference of doubled or isolated ally pawns vs enemy pawns
 */
U64 doubled_isolated(const Board *board, U64 allies, U64 enemies)
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
    return di_value;
}

/**
 * pawns
 *  evaluate pawns in the current position
 */
int pawns(const Board *board)
{
    int i;
    int pawn_value;
    U64 allies, enemies;

    allies = board->piecebb[PAWN] & board->colorbb[board->side];
    enemies = board->piecebb[PAWN] & board->colorbb[board->side ^ 1];

    pawn_value = blocked(board, allies, enemies);
    pawn_value += doubled_isolated(board, allies, enemies);
    pawn_value *= 50;
    pawn_value += standing(board, PAWN, advance_pst);
    return pawn_value;
}

/**
 * eval_heuristic
 *  evaluate the current board position
 */
int eval_heuristic(const Board *board)
{
    int board_eval;

    board_eval = 0;
    board_eval += material(board);
    board_eval += pawns(board);
    return board_eval;
}
