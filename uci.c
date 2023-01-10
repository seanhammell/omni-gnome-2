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

    if ((cp = strstr(cp, "moves")))
        board_parsemoves(board, cp + 6);
}

void parsego(Board *board, SearchInfo *sinfo, char *line)
{
    char *cp;
    int wturn, time, inc, depth, movestogo, movetime;

    line += 3;
    cp = line;
    
    wturn = board->side == 0;
    time = -1;
    inc = 0;
    depth = -1;
    movestogo = 30;
    movetime = -1;

    if ((cp = strstr(line, "wtime")) && wturn)
        time = atoi(cp + 6);
    if ((cp = strstr(line, "btime")) && !wturn)
        time = atoi(cp + 6);
    if ((cp = strstr(line, "winc")) && wturn)
        inc = atoi(cp + 5);
    if ((cp = strstr(line, "binc")) && !wturn)
        inc = atoi(cp + 5);
    if ((cp = strstr(line, "movestogo")))
        movestogo = atoi(cp + 10);
    if ((cp = strstr(line, "depth")) || (cp = strstr(line, "perft")))
        depth = atoi(cp + 6);
    if ((cp = strstr(line, "movetime")))
        movetime = atoi(cp + 9);

    sinfo->depth = depth;
    sinfo->tstart = search_gettimems();

    if (movetime != -1) {
        time = movetime;
        movestogo = 1;
    }
    if (time != -1) {
        sinfo->tset = 1;
        time /= movestogo;
        time -= 50;
        sinfo->tstop = sinfo->tstart + time + inc;
    }
    if (depth == -1)
        sinfo->depth = MAXDEPTH;

    if (!strncmp(line, "perft", 5)) {
        search_perft(board, sinfo, sinfo->depth);
        sinfo->tstop = search_gettimems();
        printf("\nNodes searched: %llu\n\n\n", sinfo->nodes);
        printf("==========\n");
        printf("Total time (ms)\t: %llu\n", sinfo->tstop - sinfo->tstart);
        printf("Nodes searched\t: %llu\n", sinfo->nodes);
        printf("MNodes/second\t: %.1f\n",
               sinfo->nodes / ((sinfo->tstop - sinfo->tstart) * 1000.0f));
        return;
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
    board_inithash();

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
