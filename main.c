#include "board.h"

static char *sfen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(void)
{
    Board board;
    board_parsefen(&board, sfen);
    return 0;
}
