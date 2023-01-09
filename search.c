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

    int wins;
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
 * init_node
 *  create a new MCTS node and initialize the appropriate members
 */
Node *init_node(void)
{
    int i;
    Node *node;

    node = malloc(sizeof(struct node));
    node->action = 0;
    node->child_action_count = 0;
    node->wins = 0;
    node->visits = 0;
    node->parent = NULL;
    node->child_count = 0;
    return node;
}

/**
 * free_node
 *  free a node and all of it's children
 */
void free_node(Node *node)
{
    int i;

    for (i = 0; i < node->child_count; ++i)
        free_node(node->children[i]);
    free(node);
}

/**
 * uct
 *  calculate the upper confidence bound for the given node
 */
float uct(const Node *node)
{
    float visits, win_ratio, exploration;

    visits = node->visits + 0.00001f;
    win_ratio = node->wins / visits;
    exploration = C * sqrt(log(node->parent->visits) / visits);
    return win_ratio + exploration;
}

/**
 * selection
 *  return the move for a leaf node that has not been added to the tree
 */
Move selection(const Node *node)
{
    int i;
    float child_uct, best_uct;
    Node *child, *best_child;

    if (node->child_count < node->child_action_count) {
        return node->child_actions[node->child_count];
    }

    best_uct = 0;
    for (i = 0; i < node->child_count; ++i) {
        child_uct = uct(node->children[i]);
        if (child_uct > best_uct) {
            best_uct = child_uct;
            best_child = node->children[i];
        }
    }

    return selection(best_child);
}

/**
 * expansion
 *  create the node of the specified action, adding it to the tree
 */
void expansion(Node *parent, Board *board, const Move action)
{
    Node *node;

    node = init_node();
    node->action = action;
    board_make(board, action);
    node->child_action_count = board_generate(board, node->child_actions);
    board_unmake(board, action);
    node->parent = parent;
    parent->children[parent->child_count] = node;
    ++parent->child_count;
}

/**
 * search_mcts
 *  Monte-Carlo Tree Search
 */
void search_mcts(Board *board, SearchInfo *sinfo)
{
    Node *root;

    root = init_node();
    root->child_action_count = board_generate(board, root->child_actions);
    while (root->child_count < root->child_action_count)
        expansion(root, board, root->child_actions[root->child_count]);
    free_node(root);
}
