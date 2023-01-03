#include "board.h"

static char *sfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(void)
{
    Board board;
    board_inittables();
    board_parsefen(&board, "3k4/8/8/8/q2K1P1r/8/8/8 w - - 0 1");
    setdanger(&board);
    board_display(&board);
    return 0;
}
