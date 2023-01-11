#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "search.h"
#include "eval.h"

#define WINDOW  25

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
 * quiesce
 *  extend the search until a quiet position is found
 */
int quiesce(Board *board, SearchInfo *sinfo, int alpha, int beta)
{
    int i, score, standpat;
    Move movelist[128];

    checkstop(sinfo);
    if (sinfo->stop)
        return 0;

    ++sinfo->nodes;
    if (board->quiet)
        return eval_heuristic(board);

    standpat = eval_heuristic(board);
    if (standpat >= beta)
        return beta;
    if (standpat > alpha)
        alpha = standpat;

    const int count = board_captures(board, movelist);
    for (i = 0; i < count; ++i) {
        board_make(board, movelist[i]);
        score = -quiesce(board, sinfo, -beta, -alpha);
        board_unmake(board, movelist[i]);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
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

    if (depth == 0)
        return quiesce(board, sinfo, alpha, beta);

    checkstop(sinfo);
    if (sinfo->stop)
        return 0;

    ++sinfo->nodes;
    const int count = board_generate(board, movelist);
    if (board_gameover(board, count))
        return board_terminal(board, count, depth);

    for (i = 0; i < count; ++i) {
        board_make(board, movelist[i]);
        score = -alphabeta(board, sinfo, depth - 1, -beta, -alpha);
        board_unmake(board, movelist[i]);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

/**
 * search_driver
 *  select the best move as returned by negamax
 */
void search_driver(Board *board, SearchInfo *sinfo)
{
    int i, depth, score, best;
    int alpha, beta;
    Move provisional, best_move, movelist[128];

    alpha = -INF;
    beta = INF;

    const int count = board_generate(board, movelist);
    for (depth = 1; depth <= sinfo->depth; ++depth) {
        for (;;) {
            best = -INF;
            for (i = 0; i < count; ++i) {
                board_make(board, movelist[i]);
                score = -alphabeta(board, sinfo, depth - 1, -beta, -alpha);
                board_unmake(board, movelist[i]);
                if (score > best) {
                    best = score;
                    provisional = movelist[i];
                }
            }
            if (best > alpha && best < beta)
                break;
            alpha = -INF;
            beta = INF;
        }

        alpha = best - WINDOW;
        beta = best + WINDOW;

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
