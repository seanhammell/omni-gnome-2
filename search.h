#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

typedef struct sinfo SearchInfo;

struct sinfo {
    int depth;
    U64 leafnodes;
    U64 nodes;
    int quit;
};

void search_clearinfo(SearchInfo *sinfo);

#endif  /* SEARCH_H */
