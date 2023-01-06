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
    MoveInfo movelist[256];

    if (depth == 1 && sinfo->depth != 1)
        return board_generate(board, movelist);
    
    if (depth == 0)
        return 1ull;

    const int count = board_generate(board, movelist);
    sinfo->nodes += count;
    nodes = 0;
    for (i = 0; i < count; ++i) {
        board_make(board, &movelist[i]);
        branch_nodes = search_perft(board, sinfo, depth - 1);
        nodes += branch_nodes;
        if (depth == sinfo->depth) {
            board_printmove(&movelist[i]);
            printf(" %llu\n", branch_nodes);
        }
        board_unmake(board, &movelist[i]);
    }

    return nodes;
}
