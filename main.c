#include "board.h"

static char *sfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(void)
{
    Board board;
    board_inittables();
    board_parsefen(&board, sfen);
    board_display(&board);
    return 0;
}
