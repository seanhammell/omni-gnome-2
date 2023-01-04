#include <stdio.h>
#include <string.h>

#include "uci.h"

#define MAXBUFF 2500
#define SFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define ID "id name Omni-Gnome 2.0\nid author Sean Hammell\nuciok\n"

void parsepos(Board *board, char *line)
{
    char *cp;

    line += 9;
    cp = line;

    if (!strncmp(cp, "fen", 3))
        board_parsefen(board, cp + 4);
    else
        board_parsefen(board, SFEN);
}

void uci_loop(void)
{
    printf("Omni-Gnome 2.0\n");
    char line[MAXBUFF];
    Board board;

    board_inittables();

    for (;;) {
        memset(&line, 0, MAXBUFF);
        fflush(stdout);

        if (!fgets(line, MAXBUFF, stdin) || line[0] == '\n')
            continue;

        if (!strncmp(line, "ucinewgame", 10))
            board_parsefen(&board, SFEN);
        else if (!strncmp(line, "isready", 7))
            printf("readyok\n");
        else if (!strncmp(line, "uci", 3))
            printf(ID);
        else if (!strncmp(line, "position", 8))
            parsepos(&board, line);
        else if (!strncmp(line, "display", 7))
            board_display(&board);
        else if(!strncmp(line, "quit", 4))
            break;
    }
}
