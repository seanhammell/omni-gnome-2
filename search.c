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
