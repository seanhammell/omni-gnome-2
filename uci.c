#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uci.h"

#define MAXBUFF 2500
#define MAXDEPTH 8
#define SFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define ID "id name Omni-Gnome 2.0\nid author Sean Hammell\nuciok\n"

/**
 * parsepos
 *  set the board position based on the uci command
 */
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

void parsego(Board *board, SearchInfo *sinfo, char *line)
{
    char *cp;
    int depth = -1;

    line += 3;
    cp = line;

    if ((cp = strstr(line, "depth")))
        depth =  atoi(cp + 6);

    sinfo->depth = depth;

    if (depth == -1)
        sinfo->depth = MAXDEPTH;

    if (!strncmp(line, "perft", 5)) {
        sinfo->leafnodes = search_perft(board, sinfo, sinfo->depth);
        printf("nodes searched: %llu\n", sinfo->leafnodes);
    }
}

/**
 * uci_loop
 *  read and carry out uci commands from stdin
 */
void uci_loop(void)
{
    printf("Omni-Gnome 2.0\n");
    char line[MAXBUFF];
    Board board;
    SearchInfo sinfo;

    board_inittables();

    for (;;) {
        memset(&line, 0, MAXBUFF);
        fflush(stdout);
        search_clearinfo(&sinfo);

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
        else if (!strncmp(line, "go", 2))
            parsego(&board, &sinfo, line);
        else if(!strncmp(line, "quit", 4))
            sinfo.quit = 1;

        if (sinfo.quit)
            break;
    }
}
