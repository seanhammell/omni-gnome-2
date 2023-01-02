#include "board.h"

static char *sfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(void)
{
    board_inittables();

    Board board;
    board_parsefen(&board, sfen);

    MoveInfo minfo;
    minfo.move = 12 | (28 << 6);
    minfo.attacker = PAWN;
    minfo.target = EMPTY;
    board_make(&board, &minfo);
    minfo.move = 51 | (35 << 6);
    minfo.attacker = PAWN;
    minfo.target = EMPTY;
    board_make(&board, &minfo);
    minfo.move = 28 | (35 << 6);
    minfo.attacker = PAWN;
    minfo.target = PAWN;
    board_make(&board, &minfo);
    board_unmake(&board, &minfo);

    board_display(&board);
    return 0;
}
