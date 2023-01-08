#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "search.h"

/**
 * gettimems
 *  get the current time in milliseconds
 */
U64 search_gettimems()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000ull + t.tv_usec / 1000ull;
}

/**
 * search_clearinfo
 *  reset all of the search information values
 */
void search_clearinfo(SearchInfo *sinfo)
{
    sinfo->depth = 0;
    sinfo->nodes = 0;
    sinfo->tset = 0;
    sinfo->tstart = 0;
    sinfo->tstop = 0;
    sinfo->stop = 0;
    sinfo->quit = 0;
}

/**
 * inputwaiting
 *  peek at stdin and return if there is input waiting
 */
int inputwaiting()
{
    fd_set readfds;
    struct timeval t;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    t.tv_sec = 0;
    t.tv_usec = 0;
    select(16, &readfds, 0, 0, &t);
    return FD_ISSET(STDIN_FILENO, &readfds);
}

/**
 * readinput
 *  if there is input waiting in stdin, stop searching and read it
 */
void readinput(SearchInfo *sinfo)
{
    int bytes;
    char input[256], *endc;

    bytes = -1;
    if (inputwaiting()) {
        sinfo->stop = 1;
        while (bytes < 0)
            bytes = read(STDIN_FILENO, input, 256);
        endc = strchr(input, '\n');
        if (endc)
            *endc = 0;
        if (strlen(input) > 0)
            if (!strncmp(input, "quit", 4))
                sinfo->quit = 1;
    }
}

/**
 * checkstop
 *  check whether time is up or the engine has been told to stop
 */
void checkstop(SearchInfo *sinfo)
{
    if (sinfo->tset && search_gettimems() > sinfo->tstop)
        sinfo->stop = 1;
    readinput(sinfo);
}

/**
 * search_perft
 *  test the speed and accuracy of move generation
 */
U64 search_perft(Board *board, SearchInfo *sinfo, int depth)
{
    int i;
    U64 nodes, branch_nodes;
    Move movelist[256];

    const int count = board_generate(board, movelist);

    if (depth == 1) {
        if (sinfo->depth == 1) {
            for (i = 0; i < count; ++i) {
                board_printmove(movelist[i]);
                printf(" 1\n");
            }
        }
        sinfo->nodes += count;
        return count;
    }

    nodes = 0;
    for (i = 0; i < count; ++i) {
        board_make(board, movelist[i]);
        branch_nodes = search_perft(board, sinfo, depth - 1);
        nodes += branch_nodes;
        if (depth == sinfo->depth) {
            board_printmove(movelist[i]);
            printf(" %llu\n", branch_nodes);
        }
        board_unmake(board, movelist[i]);
    }
    return nodes;
}
