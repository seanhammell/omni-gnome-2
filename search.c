#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "search.h"

#define C   2

typedef struct node Node;

struct node {
    Move action;
    int child_action_count;
    Move child_actions[128];

    float wins;
    int visits;

    Node *parent;
    int child_count;
    Node *children[128];
};

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
    Move movelist[128];

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

/**
 * alphabeta
 *  search the game tree, pruning branches guaranteed to produce a worse score
 *  than already promised by a previous branch
 */
int alphabeta(Board *board, SearchInfo *sinfo, int depth, int alpha, int beta)
{
    int i, score;
    Move movelist[128];

    checkstop(sinfo);
    if (sinfo->stop)
        return 0;

    ++sinfo->nodes;
    const int count = board_generate(board, movelist);
    if (board_gameover(board, count))
        return board_terminal(board, count);

    if (depth == 0)
        return board_evaluate(board);

    for (i = 0; i < count; ++i) {
        board_make(board, movelist[i]);
        score = -alphabeta(board, sinfo, depth - 1, -beta, -alpha);
        if (score >= beta) {
            board_unmake(board, movelist[i]);
            return beta;
        }
        if (score > alpha)
            alpha = score;
        board_unmake(board, movelist[i]);
    }
    return alpha;
}

/**
 * search_driver
 *  select the best move as returned by negamax
 */
void search_driver(Board *board, SearchInfo *sinfo)
{
    int i, depth, score, alpha, beta;
    Move provisional, best_move, movelist[128];

    for (depth = 1; depth <= sinfo->depth; ++depth) {
        const int count = board_generate(board, movelist);

        alpha = -6000;
        beta = 6000;
        for (i = 0; i < count; ++i) {
            board_make(board, movelist[i]);
            score = -alphabeta(board, sinfo, depth - 1, -beta, -alpha);
            if (score > alpha) {
                alpha = score;
                provisional = movelist[i];
            }
            board_unmake(board, movelist[i]);
        }

        checkstop(sinfo);
        if (sinfo->stop)
            break;

        best_move = provisional;
        printf("info pv ");
        board_printmove(best_move);
        printf(" depth %d time %llu nodes %llu\n",
               depth, search_gettimems() - sinfo->tstart, sinfo->nodes);
    }
    printf("bestmove ");
    board_printmove(best_move);
    printf("\n");
}
