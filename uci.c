#include <stdio.h>
#include <string.h>

#include "uci.h"

#define MAXBUFF 2500
#define SFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define ID "id name Omni-Gnome 2.0\nid author Sean Hammell\nuciok\n"

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

        if (!strncmp(line, "isready", 7))
            printf("readyok\n");
        else if (!strncmp(line, "uci", 3))
            printf(ID);
        else if (!strncmp(line, "display", 7))
            board_display(&board);
        else if(!strncmp(line, "quit", 4))
            break;
    }
}
