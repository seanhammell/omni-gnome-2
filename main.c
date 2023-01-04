#include "board.h"

#define SFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int main(void)
{
    Board board;
    board_inittables();
    board_parsefen(&board, SFEN);
    board_display(&board);
    return 0;
}
