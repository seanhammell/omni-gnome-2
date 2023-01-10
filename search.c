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
    sinfo->games = 0;
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
Node *init_node(Node *parent, Move action)
{
    int i;
    Node *node;

    node = malloc(sizeof(struct node));
    node->action = action;
    node->child_action_count = 0;
    node->wins = 0;
    node->visits = 0;
    node->parent = parent;
    node->child_count = 0;
    if (parent != NULL) {
        parent->children[parent->child_count] = node;
        ++parent->child_count;
    }
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
Node *selection(Node *node, Board *board, SearchInfo *sinfo)
{
    int i;
    float child_uct, best_uct;
    Node *child, *best_child;

    ++node->visits;

    if (node->child_count < node->child_action_count) {
        child = init_node(node, node->child_actions[node->child_count]);
        return child;
    }

    best_uct = 0;
    for (i = 0; i < node->child_count; ++i) {
        child_uct = uct(node->children[i]);
        if (child_uct > best_uct) {
            best_uct = child_uct;
            best_child = node->children[i];
        }
    }
    board_make(board, best_child->action);
    ++sinfo->nodes;

    return selection(best_child, board, sinfo);
}

/**
 * expansion
 *  generate all child actions for the node
 */
void expansion(Node *node, Board *board, SearchInfo *sinfo)
{
    board_make(board, node->action);
    node->child_action_count = board_generate(board, node->child_actions);
    ++node->visits;
    ++sinfo->nodes;
}

/**
 * simulation
 *  play a game of random moves and return the result
 */
int simulation(Board *board, SearchInfo *sinfo)
{
    int result;
    Move random_action, movelist[128];

    checkstop(sinfo);
    if (sinfo->stop)
        return 0;

    ++sinfo->nodes;
    const int count = board_generate(board, movelist);
    if (board_gameover(board, count)) {
        result = board_evaluate(board, count);
        ++sinfo->games;
        return result;
    }

    random_action = movelist[rand() % count];
    board_make(board, random_action);
    result = simulation(board, sinfo);
    board_unmake(board, random_action);
    return result;
}

/**
 * backpropagation
 *  propagate the result back up to the root node
 */
void backpropagation(Node *node, Board *board, int result)
{
    while (node->parent != NULL) {
        node->wins += result;
        board_unmake(board, node->action);
        node = node->parent;
    }
    node->wins += result;
}

/**
 * search_mcts
 *  Monte-Carlo Tree Search
 */
void search_mcts(Board *board, SearchInfo *sinfo)
{
    int i, result;
    float child_uct, best_uct;
    Move best_move;
    Node *root, *leaf_node;

    root = init_node(NULL, 0);
    root->child_action_count = board_generate(board, root->child_actions);
    board->root_side = board->side;

    while (!sinfo->stop) {
        leaf_node = selection(root, board, sinfo);
        expansion(leaf_node, board, sinfo);
        result = simulation(board, sinfo);
        backpropagation(leaf_node, board, result);
    }

    best_uct = 0;
    for (i = 0; i < root->child_count; ++i) {
        child_uct = uct(root->children[i]);
        if (child_uct > best_uct) {
            best_uct = child_uct;
            best_move = root->children[i]->action;
        }
    }

    printf("info time %llu nodes %llu games %d\n",
           search_gettimems() - sinfo->tstart, sinfo->nodes, sinfo->games);
    printf("bestmove ");
    board_printmove(best_move);
    printf("\n");

    free_node(root);
}
