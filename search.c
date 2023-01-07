#include <stdio.h>

#include "search.h"

/**
 * search_clearinfo
 *  reset all of the search information values
 */
void search_clearinfo(SearchInfo *sinfo)
{
    sinfo->depth = 0;
    sinfo->leafnodes = 0;
    sinfo->nodes = 0;
    sinfo->quit = 0;
}

/**
 * search_perft
 *  test the speed and accuracy of move generation
 */
U64 search_perft(Board *board, SearchInfo *sinfo, int depth)
{
    int i;
    U64 branch_nodes, nodes;
    Move movelist[256];

    const int count = board_generate(board, movelist);

    if (depth == 1) {
        if (sinfo->depth == 1) {
            for (i = 0; i < count; ++i) {
                board_printmove(movelist[i]);
                printf(" 1\n");
            }
        }
        return count;
    }

    sinfo->nodes += count;
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
