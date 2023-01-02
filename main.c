#include "board.h"

static char *sfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(void)
{
    Board board;
    board_parsefen(&board, sfen);

    Move move;
    move.info = 12 | (28 << 6);
    move.attacker = PAWN;
    move.target = EMPTY;
    board_make(&board, &move);
    move.info = 51 | (35 << 6);
    move.attacker = PAWN;
    move.target = EMPTY;
    board_make(&board, &move);
    move.info = 28 | (35 << 6);
    move.attacker = PAWN;
    move.target = PAWN;
    board_make(&board, &move);

    board_display(&board);
    return 0;
}
