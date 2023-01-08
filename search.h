#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

typedef struct sinfo SearchInfo;

struct sinfo {
    int depth;
    U64 nodes;

    int tset;
    U64 tstart;
    U64 tstop;

    int quit;
};

U64 search_gettimems();

void search_clearinfo(SearchInfo *sinfo);

U64 search_perft(Board *board, SearchInfo *sinfo, int depth);

#endif  /* SEARCH_H */
